#include "marocco/routing/results/L1Routing.h"

#include "hal/Coordinate/Relations.h"

#include <boost/serialization/nvp.hpp>

using namespace HMF::Coordinate;
using boost::multi_index::get;

namespace marocco {
namespace routing {
namespace results {

L1Routing::route_item_type::route_item_type()
{
}

L1Routing::route_item_type::route_item_type(L1Route const& route, target_type const& target)
	: m_route(route), m_source(), m_target(target)
{
	if (route.empty()) {
		throw std::invalid_argument("empty route in routing result");
	}

	auto const* merger = boost::get<DNCMergerOnHICANN>(&route.front());
	if (merger == nullptr) {
		throw std::invalid_argument("route does not start with DNC merger");
	}

	auto const* vline = boost::get<VLineOnHICANN>(&route.back());
	if (vline == nullptr) {
		throw std::invalid_argument("route does not end on vertical bus");
	}

	HICANNOnWafer const& route_target_hicann = route.target_hicann();
	if (target != route_target_hicann &&
	    target != ((vline->toSideHorizontal() == left) ? route_target_hicann.west()
	                                                   : route_target_hicann.east())) {
		throw std::invalid_argument(
			"target HICANN has to match end of route or adjacent HICANN");
	}

	HICANNOnWafer hicann = m_route.source_hicann();
	m_source = DNCMergerOnWafer(*merger, hicann);
}

L1Route const& L1Routing::route_item_type::route() const
{
	return m_route;
}

auto L1Routing::route_item_type::source() const -> source_type const&
{
	return m_source;
}

auto L1Routing::route_item_type::target() const -> target_type const&
{
	return m_target;
}

L1Routing::projection_item_type::projection_item_type()
{
}

L1Routing::projection_item_type::projection_item_type(
	source_type const& source,
	target_type const& target,
	edge_type const& edge,
	projection_type const& projection)
	: m_edge(edge), m_projection(projection), m_source(source), m_target(target)
{
}

auto L1Routing::projection_item_type::edge() const -> edge_type const&
{
	return m_edge;
}

auto L1Routing::projection_item_type::projection() const -> projection_type const&
{
	return m_projection;
}

auto L1Routing::projection_item_type::source() const -> source_type const&
{
	return m_source;
}

auto L1Routing::projection_item_type::target() const -> target_type const&
{
	return m_target;
}

auto L1Routing::add(L1Route const& route, target_type const& target) -> route_item_type const&
{
	auto const res = m_routes.insert(route_item_type(route, target));
	if (!res.second) {
		throw std::runtime_error("conflicting route when adding L1 routing result");
	}
	return *(res.first);
}

auto L1Routing::add(
	route_item_type const& route, edge_type const& edge, projection_type const& projection)
	-> projection_item_type const&
{
	auto const res = m_projections.insert(
		projection_item_type(route.source(), route.target(), edge, projection));
	if (!res.second) {
		throw std::runtime_error("conflicting projection when adding L1 routing result");
	}
	return *(res.first);
}

auto L1Routing::find_projections(edge_type const& edge) const
	-> iterable<projections_by_edge_type::iterator>
{
	return make_iterable(get<edge_type>(m_projections).equal_range(edge));
}

auto L1Routing::find_projections(projection_type const& projection) const
	-> iterable<projections_by_projection_type::iterator>
{
	return make_iterable(get<projection_type>(m_projections).equal_range(projection));
}

auto L1Routing::find_projections(route_item_type const& route) const
	-> iterable<projections_by_route_type::iterator>
{
	return make_iterable(
		get<source_and_target_type>(m_projections)
			.equal_range(boost::make_tuple(route.source(), route.target())));
}

auto L1Routing::find_routes(projection_item_type const& projection) const
	-> iterable<routes_by_projection_type::iterator>
{
	return make_iterable(
		get<source_and_target_type>(m_routes)
			.equal_range(boost::make_tuple(projection.source(), projection.target())));
}

auto L1Routing::find_routes_from(source_type const& source) const
	-> iterable<routes_by_source_type::iterator>
{
	return make_iterable(get<source_type>(m_routes).equal_range(source));
}

auto L1Routing::find_routes_to(target_type const& target) const
	-> iterable<routes_by_target_type::iterator>
{
	return make_iterable(get<target_type>(m_routes).equal_range(target));
}

bool L1Routing::empty() const
{
	return m_routes.empty();
}

size_t L1Routing::size() const
{
	return m_routes.size();
}

auto L1Routing::begin() const -> iterator {
	return m_routes.begin();
}

auto L1Routing::end() const -> iterator {
	return m_routes.end();
}

template <typename Archiver>
void L1Routing::route_item_type::serialize(Archiver& ar, const unsigned int /* version */)
{
	using namespace boost::serialization;
	// clang-format off
	ar & make_nvp("route", m_route)
	   & make_nvp("source", m_source)
	   & make_nvp("target", m_target);
	// clang-format on
}

template <typename Archiver>
void L1Routing::projection_item_type::serialize(Archiver& ar, const unsigned int /* version */)
{
	using namespace boost::serialization;
	// clang-format off
	ar & make_nvp("edge", m_edge)
	   & make_nvp("projection", m_projection)
	   & make_nvp("source", m_source)
	   & make_nvp("target", m_target);
	// clang-format on
}

template <typename Archiver>
void L1Routing::serialize(Archiver& ar, const unsigned int /* version */)
{
	using namespace boost::serialization;
	// clang-format off
	ar & make_nvp("routes", m_routes)
	   & make_nvp("projections", m_projections);
	// clang-format on
}

} // namespace results
} // namespace routing
} // namespace marocco

BOOST_CLASS_EXPORT_IMPLEMENT(::marocco::routing::results::L1Routing)
BOOST_CLASS_EXPORT_IMPLEMENT(::marocco::routing::results::L1Routing::route_item_type)
BOOST_CLASS_EXPORT_IMPLEMENT(::marocco::routing::results::L1Routing::projection_item_type)

#include "boost/serialization/serialization_helper.tcc"
EXPLICIT_INSTANTIATE_BOOST_SERIALIZE(::marocco::routing::results::L1Routing)
EXPLICIT_INSTANTIATE_BOOST_SERIALIZE(::marocco::routing::results::L1Routing::route_item_type)
EXPLICIT_INSTANTIATE_BOOST_SERIALIZE(::marocco::routing::results::L1Routing::projection_item_type)
