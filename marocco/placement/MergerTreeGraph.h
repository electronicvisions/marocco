#pragma once

#include <boost/graph/adjacency_list.hpp>

#include "halco/hicann/v2/merger0onhicann.h"
#include "halco/hicann/v2/merger1onhicann.h"
#include "halco/hicann/v2/merger2onhicann.h"
#include "halco/hicann/v2/merger3onhicann.h"
#include "halco/common/relations.h"

namespace marocco {
namespace placement {

/**
 * @brief Representation of the merger tree as a directed graph.
 * @note Edges are directed upwards, from the DNC mergers to level 0 mergers.
 */
class MergerTreeGraph
{
public:
	struct Merger
	{
		size_t level;
		size_t index;
	}; // Merger

	typedef halco::common::SideHorizontal Port;

	typedef Merger vertex_type;
	typedef Port edge_type;
	typedef boost::
	    adjacency_list<boost::vecS, boost::vecS, boost::directedS, vertex_type, edge_type>
	        graph_type;
	typedef graph_type::vertex_descriptor vertex_descriptor;
	typedef graph_type::edge_descriptor edge_descriptor;

	MergerTreeGraph();

	vertex_descriptor operator[](halco::hicann::v2::DNCMergerOnHICANN const& merger) const;
	vertex_descriptor operator[](halco::hicann::v2::Merger3OnHICANN const& merger) const;
	vertex_descriptor operator[](halco::hicann::v2::Merger2OnHICANN const& merger) const;
	vertex_descriptor operator[](halco::hicann::v2::Merger1OnHICANN const& merger) const;
	vertex_descriptor operator[](halco::hicann::v2::Merger0OnHICANN const& merger) const;

	/**
	 * @brief Mark merger as defect by removing all edges of the corresponding vertex.
	 */
	template <typename T>
	void remove(T const& merger);

	graph_type& graph();
	graph_type const& graph() const;

	static constexpr size_t vertices_count()
	{
		return (
			halco::hicann::v2::DNCMergerOnHICANN::size + halco::hicann::v2::Merger0OnHICANN::size +
			halco::hicann::v2::Merger1OnHICANN::size + halco::hicann::v2::Merger2OnHICANN::size +
			halco::hicann::v2::Merger3OnHICANN::size);
	}

private:
	graph_type m_graph;

}; // MergerTreeGraph

template <typename T>
void MergerTreeGraph::remove(T const& merger)
{
	clear_vertex(operator[](merger), m_graph);
}

} // namespace placement
} // namespace marocco
