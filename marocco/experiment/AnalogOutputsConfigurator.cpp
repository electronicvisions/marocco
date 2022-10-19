#include "marocco/experiment/AnalogOutputsConfigurator.h"

#include "halco/common/iter_all.h"

#include "marocco/Logger.h"

using namespace halco::hicann::v2;

namespace marocco {
namespace experiment {

AnalogOutputsConfigurator::AnalogOutputsConfigurator(
	parameter::results::AnalogOutputs const& analog_outputs)
	: m_analog_outputs(analog_outputs)
{
}

void AnalogOutputsConfigurator::configure(sthal::Wafer& hardware) const
{
	// As noted in marocco::parameter::results::AnalogOutputs care has to be taken when
	// choosing which denmem of a logical neuron to record, as only denmems with different
	// NeuronOnQuad coordinates can be recorded at the same time.
	// The correct denmem is now also selected in marocco::parameter::results::AnalogOutputs

	// First: disable all analog outputs of all allocated HICANNs
	for (auto const& hc : hardware.getAllocatedHicannCoordinates()) {
		hardware[hc].disable_aout();
		hardware[hc].analog.disable(AnalogOnHICANN(0));
		hardware[hc].analog.disable(AnalogOnHICANN(1));
	}

	// Analog output items are sorted by HICANN.
	for (auto const& item : m_analog_outputs) {
		auto const& logical_neuron = item.logical_neuron();
		auto const& analog_output = item.analog_output();
		auto const& hicann = item.hicann();
		auto const& recorded_denmem = item.recorded_denmem();

		auto& chip = hardware[hicann];
		chip.analog.enable(analog_output);

		MAROCCO_INFO(
		    "Recording " << logical_neuron << " via " << recorded_denmem << " and "
		                 << analog_output);
		chip.enable_aout(recorded_denmem, analog_output);
	}
}

} // namespace experiment
} // namespace marocco
