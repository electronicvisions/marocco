#include "marocco/routing/Route.h"

#include <stdexcept>
#include <set>

namespace marocco {
namespace routing {

bool Route::hasLocalSource(int const localRank, graph_t const& graph) const
{
	//return (boost::source(mProjection, graph).owner == rank);
	//FIXME
	throw std::runtime_error("fixme");
	return true;
}

bool Route::hasLocalTargets(int const localRank, graph_t const& g) const
{
	//for (auto const& proj : projections())
	//{
		//graph_t::edge_descriptor e = proj.projection();
		//if (boost::target(e, g).owner == localRank) {
			//return true;
		//}
	//}
	throw std::runtime_error("fixme");
	return true;
}

std::vector<typename Route::Projection>&
Route::projections()
{
	return *mProjections;
}

std::vector<typename Route::Projection> const&
Route::projections() const
{
	return *mProjections;
}

typename Route::BusSegment&
Route::source()
{
	return mSource;
}

typename Route::BusSegment const&
Route::source() const
{
	return mSource;
}

Route::Segments const& Route::getSegments() const
{
	return *mSegments;
}

void Route::setSegments(Segments const& s)
{
	Segments tmp(s);
	setSegments(std::move(tmp));
}

void Route::setSegments(Segments&& s)
{
	*mSegments = std::move(s);

	// calculate total length
	std::set<BusSegment> all;
	for (auto const& el : *mSegments)
	{
		for (auto const& segment : el.second)
			all.insert(segment);
	}
	mLength = all.size();
}

Route::Route(BusSegment const source,
			 std::vector<Projection> const& projections) :
	mLength(0),
	mSource(source),
	mSegments(new Segments),
	mProjections(new std::vector<Projection>(projections))
{}

size_t Route::length() const
{
	return mLength;
}

bool Route::empty() const
{
	return mSegments->empty();
}

size_t Route::numSources() const
{
	// note, that the number of sources can be LARGER, than the actual number of
	// presynaptic neurons associated with this route. Because neurons can
	// contribute as source in multiple projections.
	// But this is ok, because on the target side, we need to allocate
	// individual synapse rows for each contribution of a neuron.

	size_t num = 0;
	for (auto const& p : projections()) {
		num += p.source().size();
	}

	return num;
}

void Route::check(routing_graph const& g) const
{
	if (projections().empty())
		throw std::runtime_error("no projections associated with this route");

	if (getSegments().empty() || length()==0)
		throw std::runtime_error("no L1 Busses associated with this route");

	for (auto const& predecessors : getSegments()) {
		// last entry in predecessor (tracing route from target to source) list
		// should be the source segment.
		if (predecessors.second.back() != source())
			throw std::runtime_error("route inconsistency");
	}

	// make sure, source segment corresponds to sending repeater
	L1Bus const& bus = g[source()];
	using namespace HMF::Coordinate;
	HRepeaterOnHICANN rep(left, HLineOnHICANN(bus.getBusId()));
	if (!rep.isSending()) {
		throw std::runtime_error("source must be sending repeater");
	}

	// make sure we have no overlapping addresses
	//typedef assignment::AddressMapping::address_type address_type;
	//std::array<bool, address_type::end> contains;
	//for (auto&val : contains) { val = false; }

	//for(auto const& proj : projections())
	//{
		//for (auto const addr : proj.source().addresses())
		//{
			//// FIXME: this makes no sense, because there are multipe
			//// projectionos originating from same neurons, obviously using same
			//// addresses
			//auto& val = contains.at(addr);
			//if (val) {
				//throw std::runtime_error("lalalal");
			//}
			//val = true;
		//}
	//}
}

Route::Route() : mLength(0), mSegments(new Segments)
{}

} // namespace routing
} // namespace marocco
