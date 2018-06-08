#pragma once

#include <sstream>
#include <stdexcept>
#include <vector>
#include <array>
#include <unordered_map>
#include "metis.h"
#include "marocco/util.h"

#include <boost/graph/use_mpi.hpp>
#include <boost/graph/distributed/adjacency_list.hpp>
#include <boost/graph/distributed/mpi_process_group.hpp>
#include <boost/graph/subgraph.hpp>

namespace marocco {
namespace partition {

template<typename Graph>
class Metis
{
public:
	template<typename VertexWeights, typename EdgeWeights>
	Metis(Graph const& graph, VertexWeights const& vweight, EdgeWeights const& eweight);

	std::vector<idx_t> run(size_t parts);

	void seed(idx_t seed);

private:
	idx_t num_vertices;
	idx_t num_edges;

	std::vector< std::unordered_map<idx_t, idx_t> > pre_adjncy;
	std::vector<idx_t> adjncy; //<! adjacency edge list (CSR)
	std::vector<idx_t> xadj;   //<! vertex offset table (CSR)
	std::vector<idx_t> vwgt;   //<! vertex weight
	std::vector<idx_t> adjwgt; //<! edge weight

	std::array<idx_t, METIS_NOPTIONS> options;

	void set_options();

	void add_edge(size_t source, size_t target, int weight);

	void debug(idx_t* array, int length, const char* prefix=NULL);
}; // Metis



template<typename VertexDescriptor, typename = void>
struct vertex_index_trait;

template<typename VertexDescriptor>
struct vertex_index_trait<
	VertexDescriptor,
	typename std::enable_if<!std::is_integral<VertexDescriptor>::value>::type>
{
	template<typename Vertex>
	unsigned int operator() (Vertex const& v) const {
		return v.local;
	}
};


template<typename VertexDescriptor>
struct vertex_index_trait<
	VertexDescriptor,
	typename std::enable_if<std::is_integral<VertexDescriptor>::value>::type>
{
	unsigned int operator() (unsigned int v) const {
		return v;
	}
};


template<typename Graph>
template<typename VertexWeights, typename EdgeWeights>
Metis<Graph>::Metis(Graph const& graph, VertexWeights const& vweight, EdgeWeights const& eweight) :
		num_vertices(boost::num_vertices(graph)),
		num_edges(boost::num_edges(graph)),
		pre_adjncy(num_vertices),
		adjncy(2*num_edges),
		xadj(num_vertices + 1),
		vwgt(num_vertices),
		adjwgt(2*num_edges),
		options()
{
	set_options();

	vertex_index_trait<typename Graph::vertex_descriptor> vidx;

	// copy vertex weights to make sure it's C-style array
	for (auto const& vertex : make_iterable(vertices(graph)))
		vwgt[vidx(vertex)] = vweight[vertex];

	// create adjacency structure
	xadj[0] = 0;
	for (auto const& edge : make_iterable(edges(graph)))
	{
		auto const src = source(edge, graph);
		auto const trg = target(edge, graph);
		int const weight = eweight[edge];
		add_edge(vidx(src), vidx(trg), weight);
	}

	size_t cnt  = 0;
	size_t xcnt = 0;
	for (auto const& v : pre_adjncy)
	{
		for (auto const& idx : v)
		{
			adjncy[cnt] = idx.first;
			adjwgt[cnt] = idx.second;

			++cnt;
		}
		xadj[++xcnt] = cnt;
	}
}


template<typename Graph>
std::vector<idx_t> Metis<Graph>::run(size_t const parts)
{
	if (parts<2)
		throw std::runtime_error("parts must be at least two");

	idx_t constraints   = 1;
	//idx_t vsize        = 1;
	idx_t* vsize        = NULL;
	idx_t partitions    = parts;

	real_t* tpwgts      = NULL;
	real_t* ubvec       = NULL;

	std::vector<idx_t> partitioning(num_vertices);

	idx_t edgecut;

	int ret = METIS_PartGraphKway(
					  &num_vertices,
					  &constraints,
					  xadj.data(),
					  adjncy.data(),
					  vwgt.data(), vsize, adjwgt.data(),
					  &partitions,
					  tpwgts, ubvec, options.data(),
					  &edgecut,
					  partitioning.data());


	switch (ret) {
		case METIS_OK:
			break;
		case METIS_ERROR_INPUT:
		case METIS_ERROR_MEMORY:
		case METIS_ERROR:
			std::stringstream err;
			err << "Metis error (code: " << ret << ")";
			throw std::runtime_error(err.str());
	}

	printf("edgecut: %d\n partitions: %lu\n", edgecut, parts);
	debug(partitioning.data(), num_vertices, "partitioning: ");

	return partitioning;
}


template<typename Graph>
void Metis<Graph>::seed(idx_t const seed)
{
	options[METIS_OPTION_SEED] = seed;
}


template<typename Graph>
void Metis<Graph>::debug(idx_t* array, int length, const char* prefix)
{
	if (prefix) printf("%s", prefix);
	for (int ii=0; ii<length; ++ii) {
		printf("%d ", array[ii]);
	}
	printf("\n");
}

template<typename Graph>
void Metis<Graph>::set_options()
{
	METIS_SetDefaultOptions(options.data());

	// Possible options include

	// partioning objective, either:
	//	METIS_OBJTYPE_CUT         edge cut minimization
	//  METIS_OBJTYPE_VOL         minimize communication volume
	//options[METIS_OPTION_OBJTYPE] = METIS_OBJTYPE_CUT;

	// matching scheme
	//options[METIS_OPTION_CTYPE] = METIS_CTYPE_SHEM;

	// Determines the algorithm used during initial partitioning. Possible
	// values are:
	// METIS_IPTYPE_GROW          Grows a bisection using a greedy strategy.
	// METIS_IPTYPE_RANDOM        Computes a bisection at random followed by a refinement.
	// METIS_IPTYPE_EDGE          Derives a separator from an edge cut.
	// METIS_IPTYPE_NODE          Grow a bisection using a greedy node-based strategy.
	//options[METIS_OPTION_IPTYPE] = METIS_IPTYPE_GROW;

	// Determines the algorithm used for refinement. Possible values are:
	//	* METIS_RTYPE_FM          FM-based cut refinement.
	//	* METIS_RTYPE_GREEDY      Greedy-based cut and volume refinement.
	//	* METIS_RTYPE_SEP2SIDED   Two-sided node FM refinement.
	//	* METIS_RTYPE_SEP1SIDED   One-sided node FM refinement.
	//options[METIS_OPTION_RTYPE] = METIS_RTYPE_GREEDY;

	// Specifies the number of different partitionings that it will compute. The
	// final partitioning is the one that achieves the best edgecut or
	// communication volume
	//options[METIS_OPTION_NCUTS] = 1;

	// Specifies the number of iterations for the refinement algorithms at each
	// stage of the uncoarsening process.
	//options[METIS_OPTION_NITER] = 10;

	// Specifies the maximum allowed load imbalance among the partitions. A
	// value of x indicates that the allowed load imbalance is (1 + x)/1000.
	// [default: 30]
	options[METIS_OPTION_UFACTOR] = 30;

	// Specifies that the partitioning routines should try to minimize the
	// maximum degree of the subdomain graph
	//options[METIS_OPTION_MINCONN] = false;

	// Specifies that the partitioning routines should try to produce
	// partitions that are contiguous. Note that if the input graph is not
	// connected this option is ignored.
	//options[METIS_OPTION_CONTIG] = false;

	// RNG seed
	options[METIS_OPTION_SEED] = 42;

	// C-style numbering, starting from 0
	//options[METIS_OPTION_NUMBERING] = 0;

	// debug level
	//options[METIS_OPTION_DBGLVL] = 0;
}


template<typename Graph>
void Metis<Graph>::add_edge(size_t source, size_t target, int weight)
{
	std::unordered_map<idx_t, idx_t>& source_map = pre_adjncy[source];
	std::unordered_map<idx_t, idx_t>& target_map = pre_adjncy[target];

	auto it = source_map.find(target);
	if (it != source_map.end()) {
		weight += it->second;
	}

	source_map[target] = weight;
	target_map[source] = weight;
}

} // namespace partition
} // namespace marocco
