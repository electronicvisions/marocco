#include "marocco/experiment/Experiment.h"

#include <fstream>
#include <memory>
#include <boost/filesystem.hpp>

#include "sthal/DontProgramFloatingGatesHICANNConfigurator.h"
#include "sthal/ESSHardwareDatabase.h"
#include "sthal/ESSRunner.h"
#include "sthal/ExperimentRunner.h"
#include "sthal/HICANNConfigurator.h"
#include "sthal/HICANNv4Configurator.h"
#include "sthal/MagicHardwareDatabase.h"

#include "marocco/Logger.h"

using namespace HMF::Coordinate;

namespace marocco {
namespace experiment {

Experiment::Experiment(
	sthal::Wafer& hardware,
	results::Marocco const& results,
	BioGraph const& bio_graph,
	parameters::Experiment const& parameters,
	pymarocco::PyMarocco const& pymarocco,
	sthal::ExperimentRunner& experiment_runner,
	sthal::HardwareDatabase& hardware_database,
	sthal::HICANNConfigurator& configurator)
	: m_hardware(hardware),
	  m_results(results),
	  m_bio_graph(bio_graph),
	  m_parameters(parameters),
	  m_pymarocco(pymarocco),
	  m_experiment_runner(experiment_runner),
	  m_hardware_database(hardware_database),
	  m_configurator(configurator)
{
}

void Experiment::run()
{
	m_hardware.connect(m_hardware_database);
	m_hardware.configure(m_configurator);

	// additional record time of 1000us cf. c/1449 and c/1584
	double const record_duration_in_s = m_parameters.hardware_duration_in_s() + 1000e-6;

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

	for (auto const& spike : received_spikes) {
		if (spike.addr == l1_address) {
			spikes.push_back((spike.time - offset_in_s) * speedup);
		}
	}

	for (auto const& spike : sent_spikes) {
		if (spike.addr == l1_address) {
			spikes.push_back((spike.time - offset_in_s) * speedup);
		}
	}

	return true;
}

void Experiment::extract_results(ObjectStore& objectstore) const
{
	// In principle we could just access populations via the bio graph.  But as we store
	// pointers to const populations and the result / graph classes are provided to this
	// class via const-ref, it would not be obvious that populations are modified here.
	for (auto const& population : objectstore.populations()) {
		for (auto const& item : m_results.placement.find(population->id())) {
			extract_spikes(population, item);
		}
	}
}

} // namespace experiment
} // namespace marocco
