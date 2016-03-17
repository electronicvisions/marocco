#include "marocco/routing/WaferRouting.h"

#include <sstream>

#include "hal/Coordinate/iter_all.h"
#include "marocco/Logger.h"
#include "marocco/routing/RoutingTargetVisitor.h"
#include "marocco/routing/SynapseDriverRequirements.h"
#include "marocco/routing/SynapseLoss.h"
#include "marocco/routing/WaferRoutingPriorityQueue.h"
#include "marocco/routing/WeightMap.h"
#include "marocco/util/guess_wafer.h"
#include "marocco/util/iterable.h"
#include "marocco/util/spiral_ordering.h"
#include "marocco/util/wafer_ordering.h"

using namespace HMF::Coordinate;

namespace marocco {
namespace routing {


size_t bus_index(L1Bus const& bus)
{
	assert(bus.getDirection() == L1Bus::Vertical);
	// EM: "This looks like the calcuation of the 'horizontal bus'
	// which connects to this VLine." (gerrit #205)
	size_t const id = bus.getBusId();
	return (id / 4) % 8 + (bus.side() == HMF::Coordinate::right ? 8 : 0);
}

void BusUsage::increment(HMF::Coordinate::HICANNOnWafer const& hicann, L1Bus const& bus)
{
	++mData[hicann.id()][bus_index(bus)];
}

size_t BusUsage::get(HMF::Coordinate::HICANNOnWafer const& hicann, L1Bus const& bus) const
{
	return mData[hicann.id()][bus_index(bus)];
}

WaferRouting::~WaferRouting()
{}

std::unordered_set<HICANNGlobal> WaferRouting::get_targets(
	placement::NeuronPlacementResult::primary_denmems_for_population_type const& revmap,
	std::vector<HardwareProjection> const& projections) const
{
	std::unordered_set<HICANNGlobal> targets;

	for (auto const& hw_proj : projections)
	{
		auto const& edge = hw_proj.projection();

		graph_t::vertex_descriptor target_pop = boost::target(edge, getGraph());

		// make sure target is a neuron population rather than spike input
		if (is_source(target_pop, getGraph()))
			throw std::runtime_error("target population is a spike source");

		// further make sure it's placed somewhere
		if (revmap.find(target_pop) == revmap.end())
			throw std::runtime_error("target population is not placed");

		auto const& hw_targets = revmap.at(target_pop);

		// make sure we have some real target placed to some neuron
		if (hw_targets.empty())
		{
			MAROCCO_ERROR(is_source(boost::source(edge, getGraph()), getGraph())
				<< " " << is_source(target_pop, getGraph()));
			throw std::runtime_error("no placement of target pop");
		}

		auto const wafer = guess_wafer(getManager());
		std::transform(
			hw_targets.begin(), hw_targets.end(), std::inserter(targets, targets.begin()),
			[&wafer](NeuronOnWafer const& primary_neuron) {
				return HICANNGlobal(primary_neuron.toHICANNOnWafer(), wafer);
			});
	}

	return targets;
}

CrossbarRoutingResult
WaferRouting::run(placement::Result const& placement)
{
	// apart from priorization do the routing from the center to the outer parts
	// of the wafer.
	std::set<HICANNGlobal, wafer_ordering<HICANNGlobal, spiral_ordering<HICANNOnWafer> > > hicanns;
	for (auto const& hicann : getManager().allocated())
		hicanns.insert(hicann);

	placement::NeuronPlacementResult const& neuron_mapping = placement.neuron_placement;
	mOutbMapping = &placement.output_mapping;

	auto const& revmap = neuron_mapping.primary_denmems_for_population();

	WaferRoutingPriorityQueue queue(getGraph(), mPyMarocco);
	queue.insert(*mOutbMapping, hicanns);

	CrossbarRoutingResult res;

	while (!queue.empty())
	{
		auto const source = queue.source();
		HICANNGlobal const& hicann = source.first;
		DNCMergerOnHICANN const& dnc = source.second;
		std::vector<HardwareProjection> const projections = queue.projections();
		auto targets = get_targets(revmap, projections);

		queue.pop();

		std::vector<HICANNGlobal> to_remove;
		for (auto const& target : targets) {

			// TODO(#1594): determination whether route has synapes to target does not
			// need to count the total number of synapses.
			SynapseTargetMapping syn_tgt_mapping;
			syn_tgt_mapping.simple_mapping(
				neuron_mapping.denmem_assignment().at(target), getGraph());
			SynapseDriverRequirements req(target, neuron_mapping, syn_tgt_mapping);
			auto const num = req.calc(projections, getGraph());

			if (!num.first) {
				to_remove.push_back(target);
			}
		}
		for (auto const& target : to_remove) {
			targets.erase(target);
		}

		if (targets.empty()) {
			continue;
		}

		routing_graph::vertex_descriptor source_segment =
			mWaferGraph.getSendingL1(hicann, dnc);

		// do the actual maze routing
		std::unordered_set<HICANNGlobal> unreachable;
		auto segments = allocateRoute(
			source_segment, targets,
			unreachable);

		// construct route
		// FIXME: `projections` may still contain Projections from unreachable
		// hicanns. not realy bad, fix some time...
		Route route(source_segment, projections);
		route.setSegments(std::move(segments));

		// count synapse loss
		static bool const count_synapse_loss = true;
		if (!unreachable.empty() && count_synapse_loss) {
			auto const& sources = mOutbMapping->at(hicann).at(dnc);
			// we are hitting routing resource limits
			handleSynapseLoss(hicann, sources, unreachable, neuron_mapping, revmap);
		}

		// No possible route for any of the targets of this projection found
		if (route.empty()) {
			MAROCCO_INFO("route dropped completely");
			if (unreachable.empty()) {
				throw std::runtime_error("route without potential targets");
			}
			continue;
		}

		// test route integrity before insertion
		route.check(mRoutingGraph);

		// add source hicann for global routing, because targets can live in
		// multiple processes and we need a single responsible.

		for (auto const& segments : route.getSegments())
		{
			HICANNGlobal const& target_hicann = segments.first;
			assert(getManager().has(target_hicann));
			assert(segments.second.size() > 0);
			res[target_hicann].push_back(LocalRoute(route, segments.second[0]));
		}
	}

	// set actually set crossbar switches on hardware representation
	if (mPyMarocco.routing.is_default()) {
		configureHardware(res);
	} else {
		MAROCCO_WARN("non-default routing config => no hardware config");
	}

	size_t const synapse_loss = mSynapseLoss->getTotalLoss();
	MAROCCO_INFO("Synapse loss after WaferRouting " << synapse_loss);
	mPyMarocco.stats.setSynapseLossAfterWaferRouting(synapse_loss);

	return res;
}

void WaferRouting::clear_segment(Route::BusSegment const v)
{
	// don't use remove_vertex. It will reorganize the whole graph and all
	// descriptors will be invalidated.
	boost::clear_vertex(v, mRoutingGraph);
}

routing_graph const&
WaferRouting::getRoutingGraph() const
{
	return mRoutingGraph;
}

void WaferRouting::configureHardware(CrossbarRoutingResult const& routing)
{
	//if (getHardware().size() > 1) {
		//throw std::runtime_error("WaferRouting currently only supports single Wafer routing");
	//}

	// NOTE: because routes are stored at target chips, they might occure
	// multiple times, hence their corresponding crossbar switches might get set
	// multiple times. In the future we could cache this or simply don't care.
	for (auto cbr : routing)
	{
		HICANNGlobal const& target_hicann = cbr.first;
		std::vector<LocalRoute> const& route_list = cbr.second;

		MAROCCO_INFO("configure " << cbr.first
			<< " for " << route_list.size() << " routes.");

		for (auto const& local_route : route_list)
		{
			MAROCCO_INFO("got route with " << local_route.route().length()
				<< " L1 segments");

			auto const& segments = local_route.route().getSegments().at(target_hicann);
			auto it = segments.cbegin();

			if (local_route.targetBus() != *it) {
				throw std::runtime_error("inconsistent local route");
			}

			Route::BusSegment curr, prev = *(it++);

			// note that the segments are ordered from target to source,
			// which is relevant for setting the Repeater direction
			// accordingly. Meaning, that `prev` is always "closer" to the
			// target than `curr`.
			for (; it<segments.cend(); ++it)
			{
				curr = *it;

				L1Bus const& trg_bus = mRoutingGraph[prev];
				L1Bus const& src_bus = mRoutingGraph[curr];

				bool const final = ((it+1)==segments.cend());
				if (src_bus.getDirection() != trg_bus.getDirection()) {
					// C R O S S B A R S
					configureCrossbar(src_bus, trg_bus);
				} else {
					// R E P E A T E R

					// have we reached the last element (must be sending repeater)
					configureRepeater(src_bus, trg_bus, final);
				}

				if (final) {
					configureSPL1Repeater(src_bus, trg_bus);
				}

				// swap for next iteration
				prev = curr;
			}
		}

	} // for all target chips
}

void WaferRouting::configureCrossbar(
	L1Bus const& src,
	L1Bus const& trg)
{
	assert(src.hicann() == trg.hicann());

	MAROCCO_DEBUG("configuring crossbar");

	// C R O S S B A R S
	auto& chip = getHardware()[trg.hicann()];
	if (src.getDirection() == L1Bus::Horizontal
		&& trg.getDirection() == L1Bus::Vertical) {

		// we have a crossing from horizontal to vertical
		chip.crossbar_switches.set(trg.toVLine(), src.toHLine(), true);

	} else if (src.getDirection() == L1Bus::Vertical
		&& trg.getDirection() == L1Bus::Horizontal) {

		// we have a crossing from vertical to horizontal
		chip.crossbar_switches.set(src.toVLine(), trg.toHLine(), true);
	} else {
		throw std::runtime_error("not a crossbar");
	}
}

void WaferRouting::configureSPL1Repeater(
	L1Bus const& src,
	L1Bus const& trg)
{
	MAROCCO_DEBUG("configure SPL1 repeater ");
	if (src.getDirection() != L1Bus::Horizontal) {
		throw std::runtime_error("non horizontal final repeater");
	}

	SideHorizontal direction;
	if (src.getDirection() != trg.getDirection()) {
		// crossbar case
		direction = right;
	} else {
		// horizontal case
		HICANNGlobal const& chicann = src.hicann();
		HICANNGlobal const& phicann = trg.hicann();
		assert(chicann != phicann);

		direction = phicann.x() > chicann.x() ? right : left;
	}

	HRepeaterOnHICANN const coord(src.toHLine(), left);
	getHardware()[src.hicann()].repeater[coord].setOutput(direction, true);

	if (!coord.isSending()) {
		throw std::runtime_error("non-sending final repeater");
	}
}

void WaferRouting::configureRepeater(
	L1Bus const& src,
	L1Bus const& trg,
	bool final)
{
	MAROCCO_DEBUG("configuring repeater");

	HICANNGlobal const& phicann = trg.hicann();
	HICANNGlobal const& chicann = src.hicann();
	assert(phicann != chicann);

	// we have a straight connection. We ONLY need to set
	// the repeater if it is local to `chip`. Otherwise it
	// will be set later on in the iteration.
	if (src.getDirection() == L1Bus::Horizontal) {
		assert(trg.getDirection() == L1Bus::Horizontal);

		// on  LEFT HICANN side HORIZONTAL repeaters are on: EVEN
		// on RIGHT HICANN side HORIZONTAL repeaters are on: ODD
		bool const odd = trg.getBusId()%2;
		SideHorizontal const side = odd ? right : left;
		// Remember `trg` is closer to the target and `src` closer to source.
		// If `trg` right of `src` we need to forward to right. `trg` right of
		// `src` means `phicann.x()` is bigger.
		SideHorizontal const direction = phicann.x() > chicann.x() ? right : left;

		HMF::HICANN::HorizontalRepeater rep;
		rep.setForwarding(direction);

		if (side==direction) {
			if (!final) {
				getHardware()[src.hicann()].repeater[HRepeaterOnHICANN(src.toHLine(), side)] = rep;
			}
		} else {
			getHardware()[trg.hicann()].repeater[HRepeaterOnHICANN(trg.toHLine(), side)] = rep;
		}

	} else if (src.getDirection() == L1Bus::Vertical) {
		assert(trg.getDirection() == L1Bus::Vertical);

		// on  LEFT HICANN side VERTICAL repeaters are on: TOP/ODD and BOT/EVEN
		// on RIGHT HICANN side VERTICAL repeaters are on: TOP/EVEN and BOT/ODD
		bool const right = trg.side();
		bool const odd   = trg.getBusId()%2;
		SideVertical const side = (right xor odd) ? top : bottom;
		// Remember `trg` is closer to the target and `src` closer to source.
		// If `trg` is above `src` we need to forward signals to top, otherwise
		// to bottom. `trg` above `src` means `phicann.y()` is smaller.
		SideVertical const direction = phicann.y() < chicann.y() ? top : bottom;

		HMF::HICANN::VerticalRepeater rep;
		rep.setForwarding(direction);

		if (side==direction) {
			getHardware()[src.hicann()].repeater[VRepeaterOnHICANN(src.toVLine(), side)] = rep;
		} else {
			getHardware()[trg.hicann()].repeater[VRepeaterOnHICANN(trg.toVLine(), side)] = rep;
		}
	}
}

void WaferRouting::handleSynapseLoss(
	HICANNGlobal const& source_hicann,
	std::vector<assignment::AddressMapping> const& sources,
	std::unordered_set<HICANNGlobal> const& unreachable,
	placement::NeuronPlacementResult const& neuron_mapping,
	placement::NeuronPlacementResult::primary_denmems_for_population_type const& revmap)
{
	std::ostringstream os;
	for (auto const& hicann : unreachable) {
		os << hicann << ", ";
	}
	MAROCCO_DEBUG("targets unreachable, from " << source_hicann
		<< " to (" << os.str() << ")");

	for (auto const& source : sources)
	{
		for (auto const& edge : make_iterable(out_edges(source.bio().population(), getGraph())))
		{
			graph_t::vertex_descriptor target_pop = boost::target(edge, getGraph());

			auto const& hw_targets = revmap.at(target_pop);

			for (auto const& primary_neuron : hw_targets) {
				auto neuron_block = primary_neuron.toNeuronBlockOnWafer();
				HICANNGlobal hicann(neuron_block.toHICANNOnWafer(), guess_wafer(getManager()));
				auto it = unreachable.find(hicann);
				if (it != unreachable.end()) {
					placement::OnNeuronBlock const& onb =
						neuron_mapping.denmem_assignment().at(hicann).at(neuron_block);
					auto bio = onb[primary_neuron.toNeuronOnNeuronBlock()]->population_slice();

					mSynapseLoss->addLoss(edge, source_hicann, hicann, source.bio(), bio);
				}
			} // all hw targets
		} // all out_edges
	} // all sources
}

} // namespace routing
} // namespace marocco
