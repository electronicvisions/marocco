#pragma once

#include <boost/graph/adjacency_list.hpp>

#include "marocco/routing/L1Bus.h"

// NOTE: always use clear vertex and maybe edge lists, rather than vectors,
// because we have a rather dynamically chaning graph.
// Resource allocate for e.g. L1 lanes will remove edges.

namespace marocco {
namespace routing {

typedef boost::adjacency_list<
		boost::vecS,
		boost::vecS,
		boost::undirectedS,
		L1Bus // Vertex property
	> routing_graph;

} // namespace routing
} // namespace marocco
