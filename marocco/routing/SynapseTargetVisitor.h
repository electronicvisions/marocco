#pragma once

#include "euter/typedcellparametervector.h"
#include "marocco/routing/SynapseType.h"

namespace marocco {
namespace routing {

/**
 * Extracts the synapse targets (synapse types) of a neuron.
 *
 * Synapse targets are the different synapse types of a neuron to which afferent synapse
 * can be connected.
 * These are specified via pynn.Projection(..,target=<synapse_target>).
 * For the standard neuron models, the possible synapse targets are
 * "excitatory" and "inhibitory".
 *
 * The custom cell types \c IF_multicond_exp and \c EIF_multicond_exp_isfa_ista
 * allow more than two different synaptic input parameter sets, which can be
 * set as follows:
 * \code{.py}
 * p1 = pynn.Population(1, pynn.IF_multicond_exp)
 * p1.set('e_rev', [0.,-10,-80,-100])
 * p1.set('tau_syn', [5,10,15,20])
 *
 * # connnect src to synaptic input with reversal potential 0 and tau = 5 ms.
 * pynn.Projection(src,p1,...,target="0")
 * \endcode
 *
 * Here, the targets are numbers (as string), representing the index of the
 * parameters in the list of synaptic input parameters.
 *
 * @note For the multi conductance models, if the number of parameters for
 * e_rev is different from tau_syn, a runtime_error is thrown.
 */
struct SynapseTargetVisitor
{
	typedef std::vector<SynapseType> return_type;

	template <CellType N>
	using cell_t = TypedCellParameterVector<N>;

	template <CellType N>
	return_type operator()(cell_t<N> const&, size_t) const
	{
		throw std::runtime_error("unsupported cell type");
	}

	// AdEx
	return_type operator()(cell_t<CellType::EIF_cond_exp_isfa_ista> const&, size_t) const
	{
		return {SynapseType::excitatory, SynapseType::inhibitory};
	}

	// LIF
	return_type operator()(cell_t<CellType::IF_cond_exp> const&, size_t) const
	{
		return {SynapseType::excitatory, SynapseType::inhibitory};
	}

	// multi conductance AdEx
	return_type
	operator()(cell_t<CellType::EIF_multicond_exp_isfa_ista> const& v, size_t nrn) const;

	// multi conductance LIF
	return_type operator()(cell_t<CellType::IF_multicond_exp> const& v, size_t nrn) const;

private:
	/// returns a vector with n consecutive integers starting with 0.
	static return_type generate_n_targets(size_t n);
};

} // namespace routing
} // namespace marocco
