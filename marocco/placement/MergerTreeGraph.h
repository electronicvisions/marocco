#pragma once

#include <boost/graph/adjacency_list.hpp>

#include "hal/Coordinate/L1.h"
#include "hal/Coordinate/Merger0OnHICANN.h"
#include "hal/Coordinate/Merger1OnHICANN.h"
#include "hal/Coordinate/Merger2OnHICANN.h"
#include "hal/Coordinate/Merger3OnHICANN.h"

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

	typedef HMF::Coordinate::SideHorizontal Port;

	typedef Merger vertex_type;
	typedef Port edge_type;
	typedef boost::
	    adjacency_list<boost::vecS, boost::vecS, boost::directedS, vertex_type, edge_type>
	        graph_type;
	typedef graph_type::vertex_descriptor vertex_descriptor;
	typedef graph_type::edge_descriptor edge_descriptor;

	MergerTreeGraph();

	vertex_descriptor operator[](HMF::Coordinate::DNCMergerOnHICANN const& merger) const;
	vertex_descriptor operator[](HMF::Coordinate::Merger3OnHICANN const& merger) const;
	vertex_descriptor operator[](HMF::Coordinate::Merger2OnHICANN const& merger) const;
	vertex_descriptor operator[](HMF::Coordinate::Merger1OnHICANN const& merger) const;
	vertex_descriptor operator[](HMF::Coordinate::Merger0OnHICANN const& merger) const;

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
			HMF::Coordinate::DNCMergerOnHICANN::size + HMF::Coordinate::Merger0OnHICANN::size +
			HMF::Coordinate::Merger1OnHICANN::size + HMF::Coordinate::Merger2OnHICANN::size +
			HMF::Coordinate::Merger3OnHICANN::size);
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
