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
	HICANNOnWafer const* last_hicann = nullptr;
	NeuronOnQuad last_multiplexer_line;

	// As noted in marocco::parameter::results::AnalogOutputs care has to be taken when
	// choosing which denmem of a logical neuron to record, as only denmems with different
	// NeuronOnQuad coordinates can be recorded at the same time.

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

		auto& chip = hardware[hicann];
		chip.analog.enable(analog_output);

		// Iterate over all denmems of a logical neuron and check if it is connected to a different
		// multiplexer line compared to the last recorded denmem.  Note that only two denmems can be
		// recorded per HICANN, thus checking against the last recorded denmem on the same HICANN is
		// enough to ensure different multiplexer lines.

		bool success = false;
		for (auto const& neuron : logical_neuron) {
			auto multiplexer_line = neuron.toNeuronOnQuad();

			if (last_hicann == nullptr || *last_hicann != hicann ||
			    multiplexer_line != last_multiplexer_line) {

				MAROCCO_INFO(
					"Recording " << logical_neuron << " via " << neuron << " and " << analog_output);
				chip.enable_aout(neuron, analog_output);
				last_hicann = &(item.hicann());
				last_multiplexer_line = multiplexer_line;
				success = true;
				break;
			}
		}

		if (!success) {
			throw std::runtime_error("no free multiplexer line on HICANN");
		}
	}
}

} // namespace experiment
} // namespace marocco
