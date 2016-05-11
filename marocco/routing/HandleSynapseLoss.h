#pragma once

#include <boost/shared_ptr.hpp>

#include "hal/Coordinate/HICANN.h"
#include "hal/Coordinate/L1.h"

#include "marocco/BioGraph.h"
#include "marocco/placement/results/Placement.h"
#include "marocco/routing/results/L1Routing.h"

namespace marocco {
namespace routing {

class SynapseLoss;

class HandleSynapseLoss
{
public:
	HandleSynapseLoss(
		BioGraph const& bio_graph,
		placement::results::Placement const& neuron_placement,
		results::L1Routing const& l1_routing,
		boost::shared_ptr<SynapseLoss> const& synapse_loss);

	void operator()(results::L1Routing::route_item_type const& route_item);

	void operator()(
		HMF::Coordinate::DNCMergerOnWafer const& source_merger,
		HMF::Coordinate::HICANNOnWafer const& target_hicann,
		BioGraph::edge_descriptor const& projection);

private:
	BioGraph const& m_bio_graph;
	placement::results::Placement const& m_neuron_placement;
	results::L1Routing const& m_l1_routing;
	boost::shared_ptr<SynapseLoss> m_synapse_loss;
}; // HandleSynapseLoss

} // namespace routing
} // namespace marocco
