#pragma once

#include "sthal/Wafer.h"

#include "marocco/BioGraph.h"
#include "marocco/parameter/results/AnalogOutputs.h"
#include "marocco/placement/results/Placement.h"

namespace marocco {
namespace parameter {

/**
 * @brief Assignment of analog outputs.
 * To record the membrane of a neuron, it can be mapped to one of two analog outputs per reticle.
 */
class AnalogOutputs
{
public:
	AnalogOutputs(
		BioGraph const& bio_graph,
		placement::results::Placement const& neuron_placement);

	void run(results::AnalogOutputs& result);

private:
	BioGraph const& m_bio_graph;
	placement::results::Placement const& m_neuron_placement;
}; // AnalogOutputs

} // namespace parameter
} // namespace marocco
