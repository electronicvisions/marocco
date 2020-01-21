#include "marocco/routing/HandleSynapseLoss.h"

#include "marocco/routing/SynapseLoss.h"
#include "marocco/routing/util.h"

using namespace halco::hicann::v2;

namespace marocco {
namespace routing {

using namespace euter;

HandleSynapseLoss::HandleSynapseLoss(
	BioGraph const& bio_graph,
	placement::results::Placement const& neuron_placement,
	results::L1Routing const& l1_routing,
	boost::shared_ptr<SynapseLoss> const& synapse_loss)
	: m_bio_graph(bio_graph),
	  m_neuron_placement(neuron_placement),
	  m_l1_routing(l1_routing),
	  m_synapse_loss(synapse_loss)
{
}

void HandleSynapseLoss::operator()(results::L1Routing::route_item_type const& route_item)
{
	DNCMergerOnWafer const source_merger = route_item.source();
	HICANNOnWafer const target_hicann = route_item.target();
	for (auto const& proj_item : m_l1_routing.find_projections(route_item)) {
		auto edge = m_bio_graph.edge_from_id(proj_item.edge());
		operator()(source_merger, target_hicann, edge);
	}
}

void HandleSynapseLoss::operator()(
	halco::hicann::v2::DNCMergerOnWafer const& source_merger,
	halco::hicann::v2::HICANNOnWafer const& target_hicann,
	BioGraph::edge_descriptor const& projection)
{
	HICANNOnWafer const source_hicann = source_merger.toHICANNOnWafer();
	auto const source = boost::source(projection, m_bio_graph.graph());
	auto const target = boost::target(projection, m_bio_graph.graph());
	auto const& proj_view = m_bio_graph.graph()[projection];
	Connector::const_matrix_view_type const bio_weights = proj_view.getWeights();

	SynapseLossProxy syn_loss_proxy =
		m_synapse_loss->getProxy(projection, source_hicann, target_hicann);

	for (auto const& target_item : m_neuron_placement.find(target)) {
		auto neuron_block = target_item.neuron_block();
		if (neuron_block == boost::none || neuron_block->toHICANNOnWafer() != target_hicann) {
			// This neuron of the target population was not placed to the current hicann,
			// so there is nothing to do.
			continue;
		}

		if (!proj_view.post().mask()[target_item.neuron_index()]) {
			continue;
		}

		size_t const trg_neuron_in_proj_view = to_relative_index(
			proj_view.post().mask(), target_item.neuron_index());

		for (auto const& source_item : m_neuron_placement.find(source)) {
			auto const& address = source_item.address();
			// Only process source neuron placements matching current route.
			if (address == boost::none ||
				address->toDNCMergerOnWafer() != source_merger) {
				continue;
			}

			if (!proj_view.pre().mask()[source_item.neuron_index()]) {
				continue;
			}

			size_t const src_neuron_in_proj_view = to_relative_index(
				proj_view.pre().mask(), source_item.neuron_index());

			double const weight =
				bio_weights(src_neuron_in_proj_view, trg_neuron_in_proj_view);

			// Only record loss for synapses that were non-null to start with.
			if (std::isnan(weight) || weight <= 0.) {
				continue;
			}

			syn_loss_proxy.addLoss(src_neuron_in_proj_view, trg_neuron_in_proj_view);
		}
	}
}

} // namespace routing
} // namespace marocco
