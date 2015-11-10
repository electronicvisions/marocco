#pragma once

#include <string>
#include <vector>
#include <boost/serialization/nvp.hpp>
#include <boost/graph/adj_list_serialize.hpp>

#include "hal/Coordinate/HMFGeometry.h"
#include "marocco/routing/Result.h"
#include "marocco/routing/routing_graph.h"

// hack, otherwise wrapping was missing
typedef std::vector<HMF::Coordinate::SynapseDriverOnHICANN>
	SynapseDriverList;

class PyRoQt
{
public:
	typedef HMF::Coordinate::HICANNGlobal Index;
	typedef marocco::routing::CrossbarRoutingResult CrossbarRouting;
	typedef marocco::routing::SynapseRowRoutingResult SynapseRowRouting;
	typedef marocco::routing::routing_graph Graph;

	PyRoQt();
	PyRoQt(CrossbarRouting const& cb,
		   SynapseRowRouting const& sr,
		   Graph const& g);

	void load(std::string const& fname);
	void store(std::string const& fname) const;

	CrossbarRouting const& crossbar() const;
	SynapseRowRouting const& synapserow() const;

	Graph const& graph() const;

	marocco::routing::L1Bus const&
	getL1Bus(Graph::vertex_descriptor const& v) const;

private:
	friend class boost::serialization::access;
	template<typename Archiver>
	void serialize(Archiver& ar, unsigned int const)
	{
		using boost::serialization::make_nvp;
		ar & make_nvp("crossbar_routing", mCrossbarRouting)
		   & make_nvp("synapserow_routing", mSynapseRowRouting)
		   & make_nvp("routing_graph", mRoutingGraph);
	}

	CrossbarRouting mCrossbarRouting;
	SynapseRowRouting mSynapseRowRouting;
	Graph mRoutingGraph;
};

namespace boost {
namespace detail {

// Serialization for edge_base / endge_desc_impl is only defined in pBGL, see
// <boost/graph/distributed/adjacency_list.hpp>.

template <typename Archiver, typename Directed, typename Vertex>
void serialize(Archiver& ar, edge_base<Directed, Vertex>& e, const unsigned int /*version*/)
{
	ar & e.m_source
	   & e.m_target;
}

template <typename Archiver, typename Directed, typename Vertex>
void serialize(Archiver& ar, edge_desc_impl<Directed, Vertex>& e, const unsigned int /*version*/)
{
#ifndef PYPLUSPLUS
	// casting is necessary because e.m_eproperty is void*.
	auto unsafe_serialize = [](void*& x) -> std::intptr_t& {
		return reinterpret_cast<std::intptr_t&>(x);
	};
	ar & boost::serialization::base_object<edge_base<Directed, Vertex> >(e)
	   & unsafe_serialize(e.m_eproperty);
#endif // !PYPLUSPLUS
}

} // namespace detail
} // namespace boost
