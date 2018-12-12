#include "marocco/experiment/Experiment.h"

#include <fstream>
#include <initializer_list>
#include <memory>
#include <boost/filesystem.hpp>

#include "hal/Coordinate/HICANN.h"
#include "hal/Coordinate/External.h"
#include "hal/Coordinate/iter_all.h"

#include "sthal/DontProgramFloatingGatesHICANNConfigurator.h"
#include "sthal/ESSHardwareDatabase.h"
#include "sthal/ESSRunner.h"
#include "sthal/ExperimentRunner.h"
#include "sthal/HICANNConfigurator.h"
#include "sthal/HICANNv4Configurator.h"
#include "sthal/MagicHardwareDatabase.h"

#include "marocco/Logger.h"
#include "marocco/experiment/RecordSpikesVisitor.h"

using namespace HMF::Coordinate;

namespace marocco {
namespace experiment {

Experiment::Experiment(
	sthal::Wafer& hardware,
	results::Marocco const& results,
	BioGraph const& bio_graph,
	parameters::Experiment const& parameters,
	pymarocco::PyMarocco const& pymarocco,
	sthal::ExperimentRunner& experiment_runner)
	: m_hardware(hardware),
	  m_results(results),
	  m_bio_graph(bio_graph),
	  m_parameters(parameters),
	  m_pymarocco(pymarocco),
	  m_experiment_runner(experiment_runner)
{
	m_hardware.commonFPGASettings()->setPLL(m_pymarocco.pll_freq);

	for (auto fpga_on_wafer : m_hardware.getAllocatedFpgaCoordinates()) {
		for (auto hicann_on_dnc : iter_all<HICANNOnDNC>()) {
			auto const hicann_on_wafer = hicann_on_dnc.toHICANNOnWafer(fpga_on_wafer);

			if (!m_results.resources.has(hicann_on_wafer)) {
				MAROCCO_DEBUG("Blacklisting " << hicann_on_wafer);
				m_hardware[fpga_on_wafer].setBlacklisted(hicann_on_dnc, true);
			}

			HighspeedLinkOnWafer const hslink_on_wafer(
			    hicann_on_wafer.toHighspeedLinkOnDNC(), fpga_on_wafer.toDNCOnWafer());

			if (!m_results.resources.has(hslink_on_wafer)) {
				MAROCCO_DEBUG("Disabling highspeed connection for " << hicann_on_wafer);
				m_hardware[fpga_on_wafer].setHighspeed(hicann_on_dnc, false);
			}
		}
	}
}

void Experiment::run()
{
	double record_duration_in_s = m_parameters.hardware_duration_in_s();

	// additional record time of 1000us cf. c/1449 and c/1584
	if (m_pymarocco.backend == pymarocco::PyMarocco::Backend::Hardware) {
		record_duration_in_s += 1000e-6;
	}

	// Set up analog recorders.
	for (auto const& item : m_results.analog_outputs) {
		auto const& logical_neuron = item.logical_neuron();
		auto const& aout = item.analog_output();
		auto& chip = m_hardware[logical_neuron.front().toHICANNOnWafer()];
		MAROCCO_INFO(
			logical_neuron << " will be recorded for " << record_duration_in_s << "s via "
			<< item.analog_output() << " on " << item.reticle());
		auto recorder = chip.analogRecorder(aout);
		recorder.activateTrigger(record_duration_in_s);
		m_analog_recorders.insert(std::make_pair(logical_neuron, recorder));
	}

	m_hardware.start(m_experiment_runner);
}

bool Experiment::extract_spikes(PopulationPtr population, placement_item_type const& item) const
{
	auto const& address = item.address();
	if (address == boost::none) {
		return false;
	}

	auto const& chip = m_hardware[address->toHICANNOnWafer()];

	GbitLinkOnHICANN const gbit_link(address->toDNCMergerOnHICANN());
	auto const& received_spikes = chip.receivedSpikes(gbit_link);
	auto const& sent_spikes = chip.sentSpikes(gbit_link);

	auto& spikes = population->getSpikes(item.neuron_index());
	auto const& l1_address = address->toL1Address();

	double const offset_in_s = m_parameters.offset_in_s();
	double const speedup = m_parameters.speedup();

	if (m_parameters.truncate_spike_times()) {
		float exp_duration = m_parameters.bio_duration_in_s();
		for (auto const& in_spikes : {received_spikes, sent_spikes}) {
			for (auto const& spike : in_spikes) {
				if (spike.addr != l1_address) {
					continue;
				}
				float time = (spike.time - offset_in_s) * speedup;
				if (time < 0 || time > exp_duration) {
					continue;
				}
				spikes.push_back(time);
			}
		}
	} else {
		for (auto const& in_spikes : {received_spikes, sent_spikes}) {
			for (auto const& spike : in_spikes) {
				if (spike.addr == l1_address) {
					spikes.push_back((spike.time - offset_in_s) * speedup);
				}
			}
		}
	}

	return true;
}

bool Experiment::extract_membrane(PopulationPtr population, placement_item_type const& item) const
{
	auto const& logical_neuron = item.logical_neuron();
	if (logical_neuron.is_external()) {
		return false;
	}

	auto it = m_analog_recorders.find(logical_neuron);
	if (it == m_analog_recorders.end()) {
		return false;
	}

	auto const& recorder = it->second;

	if (!recorder.hasTriggered()) {
		MAROCCO_WARN(recorder << " failed to trigger while recording " << logical_neuron);
		return false;
	}

	auto const voltages = recorder.trace();
	auto const times = recorder.getTimestamps();

	auto& trace = population->getMembraneVoltageTrace(item.neuron_index());

	static double const s_to_ms = 1e3;
	static double const V_to_mV = 1e3;
	double const shift_v = m_pymarocco.param_trafo.shift_v;
	double const alpha_v = m_pymarocco.param_trafo.alpha_v;
	double const offset_in_s = m_parameters.offset_in_s();
	double const speedup = m_parameters.speedup();

	auto v_it = voltages.begin();
	auto const v_eit = voltages.end();
	auto t_it = times.begin();
	auto const t_eit = voltages.end();

	trace.reserve(times.size());
	if (m_parameters.truncate_membrane_traces()) {
		float const exp_duration_in_ms = m_parameters.bio_duration_in_s() * s_to_ms;
		for (; v_it != v_eit && t_it != t_eit; ++v_it, ++t_it) {
			float time = (*t_it - offset_in_s) * speedup * s_to_ms;
			if (time < 0 || time > exp_duration_in_ms) {
				continue;
			}
			trace.push_back(std::make_pair(time, (*v_it - shift_v) * V_to_mV / alpha_v));
		}
	} else {
		for (; v_it != v_eit && t_it != t_eit; ++v_it, ++t_it) {
			trace.push_back(
				std::make_pair(
					(*t_it - offset_in_s) * speedup * s_to_ms, (*v_it - shift_v) * V_to_mV / alpha_v));
		}
	}

	return true;
}

void Experiment::extract_results(ObjectStore& objectstore) const
{
	MAROCCO_INFO("Extracting experiment results");

	// In principle we could just access populations via the bio graph.  But as we store
	// pointers to const populations and the result / graph classes are provided to this
	// class via const-ref, it would not be obvious that populations are modified here.
	for (auto const& population : objectstore.populations()) {
		auto const& parameters = population->parameters();

		for (auto const& item : m_results.placement.find(population->id())) {
			if (record_spikes(parameters, item.neuron_index())) {
				extract_spikes(population, item);
			}
			// As PyNN 0.7 does not provide any way to undo record_v(), we always try to
			// extract the membrane trace as recording could have been (de-)activated by
			// the user via the `analog_outputs` results object.
			extract_membrane(population, item);
		}
	}
}

} // namespace experiment
} // namespace marocco
