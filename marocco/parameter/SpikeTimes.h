#pragma once

#include "marocco/BioGraph.h"
#include "marocco/parameter/results/SpikeTimes.h"

namespace marocco {
namespace parameter {

class SpikeTimes {
public:
	/**
	 * @param experiment_duration PyNN experiment duration in ms
	 */
	SpikeTimes(
		BioGraph const& bio_graph,
		double const experiment_duration);

	void run(results::SpikeTimes& result);

private:
	BioGraph const& m_bio_graph;
	double m_experiment_duration;
}; // SpikeTimes

} // namespace parameter
} // namespace marocco
