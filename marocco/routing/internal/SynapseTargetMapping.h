#pragma once

#include "marocco/graph.h"
#include "marocco/placement/results/Placement.h"
#include "marocco/routing/results/SynapticInputs.h"

namespace marocco {
namespace routing {
namespace internal {

/**
 * Mapping of synapse targets (synapse types) to the synaptic input circuits of
 * the neurons on a HICANN.
 *
 * The mapping of synapse targets to inputs is required for routing of synapses
 * on the HICANN and for the parameter transformation of synaptic conductance
 * parameters.
 */
struct SynapseTargetMapping
{
	/**
	 * creates a simple mapping of synapse targets onto the synaptic inputs
	 * from the neurons mapped onto the HICANN.
	 *
	 * Standard Neuron Models
	 * ----------------------
	 * For the supported standard neuron models (IF_cond_exp and
	 * EIF_cond_exp_isfa_ista), on each used hardware neuron "excitatory" is
	 * mapped to the left synaptic input, and "inhibitory" to the right.
	 *
	 * Multi conductance Models
	 * ------------------------
	 * For multi conductance neurons (IF_multicond_exp,
	 * EIF_multicond_exp_isf_ista), there are many possible mappings depending
	 * on the number of conductances and the hardware neuron size.
	 *
	 * For all settings, there are two requirements:
	 * - The neuron width multiplied by 2 must be greater or equal than the number of targets.
	 * - The mapping for neurons in the top and bottom block are always equal.
	 *
	 * Depending on the number of targets, the following procedure is applied:
	 * - For 1 target: the target is mapped to all synaptic inputs
	 * - For 2 targets: the two targets are mapped to the left rsp. right input
	 *   of all connected hardware neurons, as for the standard models.
	 * - For 3 targets:
	 *    - "0" -> left input of all neurons
	 *    - "1" -> right input of even neurons
	 *    - "2" -> right input of odd neurons
	 * - For 4 targets:
	 *    - "0" -> left input of even neurons
	 *    - "1" -> right input of even neurons
	 *    - "2" -> left input of even neurons
	 *    - "3" -> right input of odd neurons
	 * Even(odd) neurons are neurons with an even(odd) X coordinate.
	 * More than 4 targets are currently not supported.
	 *
	 * Note that for 3 and 4 targets, the targets are distributed according to
	 * parity of the neurons, and not relative to the leftmost neuron in the
	 * compound neuron.
	 *
	 * The reason:
	 * Neurons of the same type are often placed to the same chip.
	 * Let's assume a neuron size of 4 and 4 synapse targets.  If a single
	 * denmem is not working, e.g. neuron(2,top), the first neuron is mapped to
	 * neuron columns [0,1], the second to [3,4], etc...  If the synapse
	 * targets were mapped relative to the leftmost neuron, this would have bad
	 * implications for the calculation of synapse driver requirements.  E.g.
	 * in order to realize synapses to target "0" to both bio neurons, one
	 * would require both even and odd half synapse rows.  Instead, with this
	 * implementation, only even half rows are required.  Hence, the provided
	 * implementation tries to minimize the diversity of (input side, half row
	 * parity) combinations that can realize synapses to a synapse target for
	 * all neurons on the chip.  A similar case is the use of compound neurons
	 * with an odd neuron width, e.g. neuron size = 6, 10, etc.
	 *
	 * Note that for 3 targets, the target "0" is strongly prioritized over
	 * targets "1" and "2", also for bigger neuron size, where in principle the
	 * inputs might be distributed more evenly to the different targets.  The
	 * reason again is minimization of (input side, column parity) combinations
	 * per synapse targets, and simplicity.
	 *
	 * Trying out other mappings is definitely worth trying. A custom mapping
	 * per Population might be also provided via pymarocco.  These might e.g.
	 * consider the fan-in per synapse type.
	 *
	 * @param[in] hicann HICANN to operate on
	 * @param[in] neuron_placement result of the neuron placement step
	 * @param[in] graph the PyNN graph of populations and projections
	 * @param[out] synaptic_inputs
	 */
	static void simple_mapping(
		halco::hicann::v2::HICANNOnWafer const& hicann,
		placement::results::Placement const& neuron_placement,
		graph_t const& graph,
		results::SynapticInputs& synaptic_inputs);
}; // SynapseTargetMapping

} // namespace internal
} // namespace routing
} // namespace marocco
