#include "marocco/parameter/SpikeTimes.h"

#include "marocco/Logger.h"
#include "marocco/util/iterable.h"

#include "marocco/parameter/SpikeInputVisitor.h"

using namespace HMF::Coordinate;

namespace marocco {
namespace parameter {

SpikeTimes::SpikeTimes(
    BioGraph const& bio_graph,
    double experiment_duration)
    : m_bio_graph(bio_graph),
      m_experiment_duration(experiment_duration)
{
}

void SpikeTimes::run(results::SpikeTimes& result)
{
	auto const& graph = m_bio_graph.graph();
	for (auto const& vertex : make_iterable(boost::vertices(graph))) {
		if (!is_source(vertex, graph)) {
			continue;
		}

		euter::Population const& pop = *(graph[vertex]);
		for (size_t neuron_id = 0, end = pop.size(); neuron_id < end; ++neuron_id) {
			BioNeuron bio_neuron(vertex, neuron_id);
			// FIXME: Combine with global hash parameter (to be introduced).
			size_t const seed = hash_value(bio_neuron);
			result.set(
			    bio_neuron, extract_input_spikes(pop, neuron_id, seed, m_experiment_duration));
		}
	}
}

} // namespace parameter
} // namespace marocco
