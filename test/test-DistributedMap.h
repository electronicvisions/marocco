#include "test/common.h"
#include "marocco/DistributedMap.h"
#include "marocco/graph.h"

namespace marocco {

typedef graph_t::vertex_descriptor key;

inline
key make_key(size_t localVertex, int rank = 0)
{
	//return key(boost::processor_id_type(rank), localVertex);
	return key( localVertex);
}

} // marocco
