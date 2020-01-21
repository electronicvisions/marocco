#pragma once

#include <set>
#include <map>
#include <vector>

#include "halco/hicann/v2/l1.h"
#include "halco/hicann/v2/neuron.h"
#include "marocco/placement/ConstrainMergers.h"
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
	typedef std::map<halco::hicann::v2::NeuronBlockOnHICANN, halco::hicann::v2::DNCMergerOnHICANN>
		result_type;

	/**
	 * @param graph Representation of merger tree.
	 *              Use MergerTreeGraph::remove() to implement hardware defects.
	 * @param nbm Result of neuron placement, used to extract the number of mapped bio
	 *            neurons for each neuron block.
	 * @param constrainer may contain a functor to check if constraints are met
	 */
	MergerTreeRouter(
	    MergerTreeGraph const& graph,
	    internal::Result::denmem_assignment_type const& nbm,
	    boost::optional<ConstrainMergers> const constrainer = boost::none);

	void run(halco::hicann::v2::HICANNOnWafer const& hicann);

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
	std::pair<std::set<halco::hicann::v2::NeuronBlockOnHICANN>,
	          std::vector<MergerTreeGraph::vertex_descriptor> >
	mergeable(
	    halco::hicann::v2::DNCMergerOnHICANN const& merger);

	/**
	 * @brief Representation of the merger tree.
	 * @note This stores a copy of the handed-in graph, as the algorithm will remove edges
	 *       to track used mergers.
	 */
	MergerTreeGraph m_graph;

	/// number of placed neurons for each NeuronBlock
	halco::common::typed_array<size_t, halco::hicann::v2::NeuronBlockOnHICANN> m_neurons;

	/// mapping of Neurons to Hardware
	internal::Result::denmem_assignment_type const& m_denmems;

	/// merger tree result
	result_type m_result;

	boost::optional<ConstrainMergers> const m_constraints_checker;
}; // MergerTreeRouter

} // namespace placement
} // namespace marocco
