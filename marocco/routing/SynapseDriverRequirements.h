#pragma once

#include <array>
#include "hal/Coordinate/HMFGeometry.h"
#include "marocco/placement/Result.h"
#include "marocco/routing/LocalRoute.h"

namespace marocco {
namespace routing {

class SynapseDriverRequirements
{
public:
	typedef std::array<size_t, 2/*exc/inh*/ * 4/*MSB*/> Histogram;

	SynapseDriverRequirements(
		HMF::Coordinate::HICANNGlobal const& hicann,
		marocco::placement::NeuronPlacementResult const& nrnpl);

	/** calculates the number of required synapse drivers to realize
	 *  all synapses from an inbound connection (a VLine)
	 *
	 *  returns the pair (nr. of required drivers, nr. of synapses)
	 *
	 *  furthermore, it fills two histograms:
	 *  @param synapse_histogram
	 *         for each combination of 2MSB pattern and synaptic input target 
	 *         (exc/inh) the number of synapses
	 *
	 *  @param synrow_histogram
	 *         for each combination of 2MSB pattern and synaptic input target 
	 *         (exc/inh) the number of half synapse row required to realize
	 *         all synapses
	 */
	std::pair<std::size_t, std::size_t>
	calc(std::vector<HardwareProjection> const& projections,
		 graph_t const& graph,
		 Histogram& synapse_histogram,
		 Histogram& synrow_histogram);

	// for compat reasons only
	std::pair<std::size_t, std::size_t>
	calc(std::vector<HardwareProjection> const& projections,
		 graph_t const& graph);

	HMF::Coordinate::HICANNGlobal const& hicann() const {
		return mHICANN;
	}

private:
	HMF::Coordinate::HICANNGlobal mHICANN;
	marocco::placement::NeuronPlacementResult const& mNeuronPlacement;
};

} // namespace routing
} // namespace marocco
