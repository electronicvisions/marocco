#include "marocco/parameter/CurrentSources.h"

#include "marocco/Logger.h"

using namespace HMF::Coordinate;

namespace marocco {
namespace parameter {

CurrentSources::CurrentSources(
	sthal::Wafer& hardware, placement::results::Placement const& neuron_placement)
	: m_hardware(hardware), m_neuron_placement(neuron_placement)
{
}

void CurrentSources::run(current_sources_type const& current_sources)
{
	std::unordered_map<HICANNOnWafer, std::pair<NeuronOnHICANN, ConstCurrentSourcePtr> >
		mapped_current_sources;

	for (auto const& entry : current_sources) {
		auto const& bio_neuron = entry.first;
		auto const& current_source = entry.second;

		// There should be exactly one result for this lookup.
		auto iterable = m_neuron_placement.find(bio_neuron);
		assert(iterable.begin() != iterable.end());
		assert(std::next(iterable.begin()) == iterable.end());

		auto const& logical_neuron = iterable.begin()->logical_neuron();
		assert(!logical_neuron.is_external());

		auto const primary_neuron = logical_neuron.front();
		auto result = mapped_current_sources.insert(
			std::make_pair(
				primary_neuron.toHICANNOnWafer(),
				std::make_pair(primary_neuron.toNeuronOnHICANN(), current_source)));
		if (!result.second) {
			MAROCCO_WARN(
				"more than one current source per HICANN; discarding input to " << bio_neuron);
		}
	}

	for (auto const& entry : mapped_current_sources) {
		auto const& hicann = entry.first;
		auto const& neuron = entry.second.first;
		auto const step_source =
			boost::dynamic_pointer_cast<StepCurrentSource const>(entry.second.second);
		if (!step_source) {
			throw std::runtime_error("unsupported current source type");
	    }

		auto const& times = step_source->times();
		auto const& amplitudes = step_source->amplitudes();

		// TODO: This should be enforced/guaranteed by pyhmf/euter
		if (times.size() != amplitudes.size()) {
			throw std::runtime_error(
				"step current input has different number of amplitudes / times.");
		}

		sthal::FGStimulus pattern;
		size_t const pattern_size = pattern.size();
		if (amplitudes.size() != pattern_size) {
			throw std::runtime_error("unsupported size of step current input pattern");
		}

		for (size_t ii = 0; ii < pattern_size; ++ii) {
			pattern[ii] = amplitudes[ii];
		}

		pattern.setPulselength(15);
		pattern.setContinuous(true);

		MAROCCO_TRACE("setting FGStimulus for " << neuron);
		m_hardware[hicann].setCurrentStimulus(neuron, pattern);
	}
}

} // namespace parameter
} // namespace marocco
