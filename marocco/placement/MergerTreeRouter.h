#pragma once

#include <set>
#include <map>
#include <vector>

#include "hal/Coordinate/L1.h"
#include "hal/Coordinate/Neuron.h"
#include "hal/Coordinate/typed_array.h"
#include "marocco/placement/MergerTreeGraph.h"
#include "marocco/placement/internal/Result.h"

namespace marocco {
namespace placement {

/**
 * @brief Tries to merge adjacent neuron blocks such that the number of DNC mergers needed
 *        is minimized.
 * Initially an attempt is made to route all neuron blocks to \c DNCMergerOnHICANN(3),
 * which is located in the center of the merger tree.  If not successful the algorithm
 * attempts to route as many adjacent neuron blocks as possible to mergers located further
 * to the sides.  In doing so an adjacent neuron block is considered for merging, if the
 * number of biological neurons (i.e. L1 addresses) does not exceed the maximum number of
 * addresses for a single L1 bus (see \c L1AddressPool::capacity()).
 * Merging of non-adjacent blocks is not allowed, to prevent “muting” the blocks in between.
 */
class MergerTreeRouter {
public:
	typedef std::map<HMF::Coordinate::NeuronBlockOnHICANN, HMF::Coordinate::DNCMergerOnHICANN>
		result_type;

	/**
	 * @param graph Representation of merger tree.
	 *              Use MergerTreeGraph::remove() to implement hardware defects.
	 * @param nbm Result of neuron placement, used to extract the number of mapped bio
	 *            neurons for each neuron block.
	 */
	MergerTreeRouter(
		MergerTreeGraph const& graph,
		internal::Result::denmem_assignment_type::mapped_type const& nbm);

	void run();

	/**
	 * @return Mapping of neuron blocks to DNC mergers.
	 * @see MergerTreeConfigurator, which accepts this as its input.
	 */
	result_type const& result() const;

private:
	/**
	 * @brief Try to route as many adjacent neuron blocks as possible to the specified merger.
	 * @return Pair of adjacent neuron blocks that were found to be mergeable and list of
	 *         predecessors that contains the path from those neuron blocks to the
	 *         specified merger.
	 */
	std::pair<std::set<HMF::Coordinate::NeuronBlockOnHICANN>,
	          std::vector<MergerTreeGraph::vertex_descriptor> >
	mergeable(
	    HMF::Coordinate::DNCMergerOnHICANN const& merger);

	/**
	 * @brief Representation of the merger tree.
	 * @note This stores a copy of the handed-in graph, as the algorithm will remove edges
	 *       to track used mergers.
	 */
	MergerTreeGraph m_graph;

	/// number of placed neurons for each NeuronBlock
	HMF::Coordinate::typed_array<size_t, HMF::Coordinate::NeuronBlockOnHICANN> m_neurons;

	result_type m_result;
}; // MergerTreeRouter

} // namespace placement
} // namespace marocco
