#include "marocco/routing/L1Routing.h"

#include <algorithm>

#include "marocco/Logger.h"
#include "marocco/routing/L1BackboneRouter.h"
#include "marocco/routing/SynapseDriverRequirements.h"
#include "marocco/routing/VLineUsage.h"
#include "marocco/routing/internal/SynapseTargetMapping.h"
#include "marocco/routing/results/SynapticInputs.h"
#include "marocco/util/algorithm.h"

using namespace HMF::Coordinate;

namespace marocco {
namespace routing {

namespace {

L1Route::segment_type to_segment(L1BusOnWafer const& bus)
{
	if (bus.is_horizontal()) {
		return bus.toHLineOnHICANN();
	}
	return bus.toVLineOnHICANN();
}

} // namespace

L1Routing::L1Routing(
	L1RoutingGraph& l1_graph,
	BioGraph const& bio_graph,
	parameters::L1Routing const& parameters,
	placement::results::Placement const& neuron_placement,
	results::L1Routing& result)
	: m_l1_graph(l1_graph),
	  m_bio_graph(bio_graph),
	  m_parameters(parameters),
	  m_neuron_placement(neuron_placement),
	  m_result(result)
{
}

std::vector<DNCMergerOnWafer> L1Routing::sources_sorted_by_priority() const
{
	typedef parameters::L1Routing::projection_type projection_type;
	typedef parameters::L1Routing::priority_type priority_type;
	auto const& graph = m_bio_graph.graph();
	std::unordered_map<DNCMergerOnWafer, std::vector<priority_type> >
		merger_priorities;

	for (auto const& item : m_neuron_placement) {
		auto const merger = item.dnc_merger();
		if (merger == boost::none) {
			continue;
		}

		if (out_degree(item.population(), graph) == 0u) {
			// No outgoing connections.
			continue;
		}

		for (auto const& edge : make_iterable(out_edges(item.population(), graph))) {
			projection_type const projection_id = graph[edge].projection()->id();
			merger_priorities[*merger].push_back(m_parameters.priority(projection_id));
		}
	}

	std::vector<DNCMergerOnWafer> result;
	result.reserve(merger_priorities.size());
	std::unordered_map<DNCMergerOnWafer, priority_type> merger_measure;
	for (auto const& item : merger_priorities) {
		result.push_back(item.first);
		switch (m_parameters.priority_accumulation_measure()) {
			case parameters::L1Routing::PriorityAccumulationMeasure::arithmetic_mean:
				merger_measure[item.first] =
					algorithm::arithmetic_mean(item.second.begin(), item.second.end());
				break;
			default:
				throw std::runtime_error("unknown priority measure");
		}
	}

	std::sort(
		result.begin(), result.end(),
		[&merger_measure](DNCMergerOnWafer const& lhs, DNCMergerOnWafer const& rhs) -> bool {
			auto const& left = merger_measure[lhs];
			auto const& right = merger_measure[rhs];
			return (left != right) ? (left > right) : (lhs < rhs);
		});

	return result;
}

std::unordered_map<HICANNOnWafer, std::set<BioGraph::edge_descriptor> >
L1Routing::targets_for_source(DNCMergerOnWafer const& merger) const
{
	std::unordered_map<HICANNOnWafer, std::set<BioGraph::edge_descriptor> > result;
	auto const& graph = m_bio_graph.graph();

	for (auto const& item : m_neuron_placement.find(merger)) {
		for (auto const& edge : make_iterable(out_edges(item.population(), graph))) {
			auto const target_population = target(edge, graph);

			if (is_source(target_population, graph)) {
				throw std::runtime_error("target population is a spike source");
			}

			auto const placements = m_neuron_placement.find(target_population);
			if (placements.empty()) {
				throw std::runtime_error("target population has not been placed");
			}

			for (auto const& placement : placements) {
				auto const neuron_block = placement.neuron_block();
				// Spike sources should never have incoming edges.
				assert(neuron_block != boost::none);
				result[neuron_block->toHICANNOnWafer()].insert(edge);
			}
		}
	}

	// Remove HICANN if no outgoing projection has target populations there, i.e. the
	// number of required synapse drivers is zero.
	for (auto it = result.begin(), eit = result.end(); it != eit;) {
		// TODO(#1594): determination whether route has synapes to target does not
		// need to count the total number of synapses.
		results::SynapticInputs synaptic_inputs;
		internal::SynapseTargetMapping::simple_mapping(it->first, m_neuron_placement, graph, synaptic_inputs);
		SynapseDriverRequirements requirements(it->first, m_neuron_placement, synaptic_inputs);
		auto const num = requirements.calc(merger, graph);

		if (num.first == 0u) {
			it = result.erase(it);
		} else {
			++it;
		}
	}

	return result;
}

void L1Routing::run()
{
	switch (m_parameters.algorithm()) {
		case parameters::L1Routing::Algorithm::backbone:
			run_backbone_router();
			break;
		default:
			throw std::runtime_error("unknown routing algorithm");
	}
}

void L1Routing::run_backbone_router()
{
	auto const& graph = m_l1_graph.graph();

	auto const sources = sources_sorted_by_priority();
	MAROCCO_DEBUG("found " << sources.size() << " sources");

	L1GraphWalker walker(graph);

	// Avoid horizontal buses belonging to used sending repeaters.
	for (auto const& merger : sources) {
		walker.avoid_using(
		    m_l1_graph[merger.toHICANNOnWafer()]
		              [merger.toSendingRepeaterOnHICANN().toHLineOnHICANN()]);
	}

	/*
	 * ,---- [ Thesis S. Jeltsch: 6.4.4 Iterative Horizontal Growth Routing ]
	 * | Whenever a chip is reached that is in the same column as any of the targets, a
	 * | vertical connection is established.  Again, all vertical buses reachable via local
	 * | crossbar switches are considered.  The best option is picked based on a score. For
	 * | each option, the score is initially set to zero and a vertical walk is started, both
	 * | upwards and downwards.
	 * | When a target chip is encountered the score is increment by two, if less than 12
	 * | other routes are already competing for the same set of synapse drivers, otherwise by
	 * | one.  The scoring is useful to reduce the number of competing routes because up to 16
	 * | vertical buses share 14 synapse drivers, see Section 1.5.3.
	 * `----
	 * Comment: The implemented score is greater by 1 w.r.t. the description in the thesis.
	 */
	VLineUsage vline_usage;
	auto scoring_function =
		([&vline_usage, &graph](L1RoutingGraph::vertex_descriptor const& vertex) -> size_t {
			auto bus = graph[vertex];
			assert(bus.is_vertical());
			return vline_usage.get(bus.toHICANNOnWafer(), bus.toVLineOnHICANN()) < 12 ? 3 : 2;
		});

	for (auto const& merger : sources) {
		auto const source = m_l1_graph[merger.toHICANNOnWafer()]
		                              [merger.toSendingRepeaterOnHICANN().toHLineOnHICANN()];
		auto const targets = targets_for_source(merger);

		MAROCCO_TRACE("routing from " << merger << " to " << targets.size() << " targets");

		L1BackboneRouter backbone(walker, source, scoring_function);

		for (auto const& target : targets) {
			backbone.add_target(target.first);
		}

		backbone.run();

		PathBundle bundle;
		for (auto const& target : targets) {
			auto const path = backbone.path_to(target.first);

			if (store_result(request_type{merger, target.first, target.second}, source, path)) {
				bundle.add(path);
			}
		}
		m_l1_graph.remove(bundle);
	}
}

bool L1Routing::store_result(
	request_type const& request,
    L1RoutingGraph::vertex_descriptor const source,
    PathBundle::path_type const& path)
{
	if (path.empty()) {
		MAROCCO_WARN(
			"could not establish route from " << request.source << " to " << request.target);
		m_failed.push_back(request);
		return false;
	}

	if (path.front() != source) {
		MAROCCO_WARN(
			"could not establish route from " << request.source << " to " << request.target
		    << " because of cycle:\n" << toL1Route(m_l1_graph.graph(), path));
		m_failed.push_back(request);
		return false;
	}

	MAROCCO_TRACE("found route to " << request.target);
	auto const route = with_dnc_merger_prefix(toL1Route(m_l1_graph.graph(), path), request.source);
	auto const& item = m_result.add(route, request.target);
	auto const& graph = m_bio_graph.graph();
	for (auto const& edge : request.projections) {
		m_result.add(item, m_bio_graph.edge_to_id(edge), graph[edge].projection()->id());
	}

	return true;
}

auto L1Routing::failed_routes() const -> std::vector<request_type> const&
{
	return m_failed;
}

L1Route toL1Route(PathBundle::graph_type const& graph, PathBundle::path_type const& path)
{
	assert(!path.empty());
	auto const& bus = graph[path.front()];
	auto current_hicann = bus.toHICANNOnWafer();

	L1Route route;
	route.append(current_hicann, to_segment(bus));

	for (auto it = std::next(path.begin()), eit = path.end(); it != eit; ++it) {
		auto const& bus = graph[*it];
		auto hicann = bus.toHICANNOnWafer();
		if (hicann != current_hicann) {
			route.append(hicann, to_segment(bus));
			current_hicann = hicann;
		} else {
			route.append(to_segment(bus));
		}
	}

	return route;
}

L1RouteTree toL1RouteTree(PathBundle::graph_type const& graph, PathBundle const& bundle)
{
	L1RouteTree tree;
	for (auto const& path : bundle.paths()) {
		tree.add(toL1Route(graph, path));
	}
	return tree;
}

L1Route with_dnc_merger_prefix(L1Route const& route, DNCMergerOnWafer const& merger)
{
	if (route.empty()) {
		return route;
	}
	assert(route.size() >= 2); // Non-empty routes have at least two elements.

	// Operate on copy, so original route is not modified if the prepend operation throws.
	L1Route result = route;
	L1Route const dnc_prefix{merger.toHICANNOnWafer(), merger.toDNCMergerOnHICANN()};
	HICANNOnWafer const hicann = route.source_hicann();

	// If we immediately switch from the source HICANN to the left, the intention is
	// to set the output mode of the sending repeater to LEFT.  This is encoded in a
	// L1Route by a DNCMergerOnHICANN directly followed by a HICANNOnWafer.
	auto it = result.begin();
	std::advance(it, 2);
	if (auto const* next_hicann = boost::get<HICANNOnWafer>(&*it)) {
		try {
			if (*next_hicann == hicann.west()) {
				// Strip the leading [HICANNOnWafer, HLineOnWafer] segments.
				result = result.split(it).second;
			}
		} catch (std::overflow_error const&) {
			// reached bound of wafer, other HICANN does not exist
		} catch (std::domain_error const&) {
			// invalid combination of X and Y (can happen because wafer is round)
		}
	}

	result.prepend(dnc_prefix, L1Route::extend_mode::extend);

	return result;
}

} // namespace routing
} // namespace marocco
