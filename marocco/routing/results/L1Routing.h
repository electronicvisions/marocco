#pragma once

#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/serialization/export.hpp>

#include "halco/hicann/v2/hicann.h"
#include "halco/hicann/v2/l1.h"

#include "marocco/coordinates/L1Route.h"
#include "marocco/routing/results/Edge.h"
#include "marocco/util/iterable.h"

namespace boost {
namespace serialization {
class access;
} // namespace serialization
} // namespace boost

namespace marocco {
namespace routing {
namespace results {

class L1Routing {
public:
	typedef size_t vertex_descriptor;
	typedef Edge edge_type;
	typedef size_t projection_type;
	typedef halco::hicann::v2::DNCMergerOnWafer source_type;
	typedef halco::hicann::v2::HICANNOnWafer target_type;
	typedef boost::tuple<source_type, target_type> source_and_target_type;

	/**
	 * @brief Single connection from a given DNC merger to another HICANN.
	 * @note In addition to the L1Route this needs to explicitly store the target, as the
	 *       end of the stored L1 route may lie on an adjacent HICANN.
	 */
	class route_item_type {
	public:
		/**
		 * @param target HICANN containing target population.
		 *               Can be different from \c route.target_hicann() when a synapse
		 *               switch on the target HICANN of the route is used to reach a
		 *               synapse driver on the HICANN containing the target population.
		 */
		route_item_type(L1Route const& route, target_type const& target);

		L1Route const& route() const;
		source_type const& source() const;
		target_type const& target() const;

	private:
		L1Route m_route;
		source_type m_source;
		target_type m_target;

		friend class boost::serialization::access;
		route_item_type();
		template <typename Archiver>
		void serialize(Archiver& ar, const unsigned int /* version */);
	}; // route_item_type

	/**
	 * @brief Projection that led to a given hardware connection.
	 * This can be used to look up the corresponding route, which is identified by its
	 * source and target.
	 * @note There can be multiple projections per route.
	 */
	class projection_item_type {
	public:
		projection_item_type(
			source_type const& source,
			target_type const& target,
			edge_type const& edge,
			projection_type const& projection);

		edge_type const& edge() const;
		projection_type const& projection() const;
		source_type const& source() const;
		target_type const& target() const;

	private:
		edge_type m_edge;
		projection_type m_projection;
		source_type m_source;
		target_type m_target;

		friend class boost::serialization::access;
		projection_item_type();
		template <typename Archiver>
		void serialize(Archiver& ar, const unsigned int /* version */);
	}; // projection_item_type

	typedef boost::multi_index::multi_index_container<
		route_item_type,
		boost::multi_index::indexed_by<
			boost::multi_index::hashed_unique<
				boost::multi_index::tag<source_and_target_type>,
				boost::multi_index::composite_key<
					route_item_type,
					boost::multi_index::
						const_mem_fun<route_item_type, source_type const&, &route_item_type::source>,
					boost::multi_index::
						const_mem_fun<route_item_type, target_type const&, &route_item_type::target> > >,
			boost::multi_index::hashed_non_unique<
				boost::multi_index::tag<source_type>,
				boost::multi_index::
					const_mem_fun<route_item_type, source_type const&, &route_item_type::source> >,
			boost::multi_index::hashed_non_unique<
				boost::multi_index::tag<target_type>,
				boost::multi_index::
					const_mem_fun<route_item_type, target_type const&, &route_item_type::target> > > >
		routes_type;
	typedef boost::multi_index::multi_index_container<
		projection_item_type,
		boost::multi_index::indexed_by<
			boost::multi_index::hashed_unique<
				boost::multi_index::composite_key<
					projection_item_type,
					boost::multi_index::const_mem_fun<projection_item_type,
					                                  edge_type const&,
					                                  &projection_item_type::edge>,
					boost::multi_index::const_mem_fun<projection_item_type,
					                                  source_type const&,
					                                  &projection_item_type::source>,
					boost::multi_index::const_mem_fun<projection_item_type,
					                                  target_type const&,
					                                  &projection_item_type::target> > >,
			boost::multi_index::hashed_non_unique<
				boost::multi_index::tag<source_and_target_type>,
				boost::multi_index::composite_key<
					projection_item_type,
					boost::multi_index::const_mem_fun<projection_item_type,
					                                  source_type const&,
					                                  &projection_item_type::source>,
					boost::multi_index::const_mem_fun<projection_item_type,
					                                  target_type const&,
					                                  &projection_item_type::target> > >,
			boost::multi_index::hashed_non_unique<
				boost::multi_index::tag<edge_type>,
				boost::multi_index::const_mem_fun<projection_item_type,
				                                  edge_type const&,
				                                  &projection_item_type::edge> >,
			boost::multi_index::hashed_non_unique<
				boost::multi_index::tag<projection_type>,
				boost::multi_index::const_mem_fun<projection_item_type,
				                                  projection_type const&,
				                                  &projection_item_type::projection> > > >
		projections_type;
	typedef routes_type::index<source_and_target_type>::type routes_by_projection_type;
	typedef routes_type::index<source_type>::type routes_by_source_type;
	typedef routes_type::index<target_type>::type routes_by_target_type;
	typedef projections_type::index<edge_type>::type projections_by_edge_type;
	typedef projections_type::index<projection_type>::type projections_by_projection_type;
	typedef projections_type::index<source_and_target_type>::type projections_by_route_type;
	typedef routes_type::iterator iterator;
	typedef routes_type::iterator const_iterator;

	route_item_type const& add(L1Route const& route, target_type const& target);
	projection_item_type const& add(
		route_item_type const& route, edge_type const& edge, projection_type const& projection);

	iterable<projections_by_edge_type::iterator> find_projections(
		edge_type const& edge) const;

	/**
	 * @brief Return all projection items corresponding to this euter projection id.
	 * As in principle there may be multiple routes for each projection, multiple projection items
	 * may be returned here.
	 */
	iterable<projections_by_projection_type::iterator> find_projections(
		projection_type const& projection) const;

	iterable<projections_by_route_type::iterator> find_projections(
		route_item_type const& route) const;

	iterable<routes_by_projection_type::iterator> find_routes(
		projection_item_type const& projection) const;

	iterable<routes_by_source_type::iterator> find_routes_from(source_type const& source) const;

	iterable<routes_by_target_type::iterator> find_routes_to(target_type const& target) const;

	bool empty() const;

	size_t size() const;

	/**
	 * @brief Return iterator to all implemented routes.
	 * @see #find_projections() to retrieve the corresponding projection.
	 */
	iterator begin() const;

	iterator end() const;

private:
	routes_type m_routes;
	projections_type m_projections;

	friend class boost::serialization::access;
	template <typename Archiver>
	void serialize(Archiver& ar, const unsigned int /* version */);
}; // L1Routing

PYPP_INSTANTIATE(iterable<L1Routing::projections_by_edge_type::iterator>)
PYPP_INSTANTIATE(iterable<L1Routing::projections_by_projection_type::iterator>)
PYPP_INSTANTIATE(iterable<L1Routing::projections_by_route_type::iterator>)
PYPP_INSTANTIATE(iterable<L1Routing::routes_by_projection_type::iterator>)
PYPP_INSTANTIATE(iterable<L1Routing::routes_by_source_type::iterator>)
PYPP_INSTANTIATE(iterable<L1Routing::routes_by_target_type::iterator>)

} // namespace results
} // namespace routing
} // namespace marocco

BOOST_CLASS_EXPORT_KEY(::marocco::routing::results::L1Routing)
BOOST_CLASS_EXPORT_KEY(::marocco::routing::results::L1Routing::route_item_type)
BOOST_CLASS_EXPORT_KEY(::marocco::routing::results::L1Routing::projection_item_type)
