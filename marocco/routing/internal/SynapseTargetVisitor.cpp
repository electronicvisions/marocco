#include "marocco/routing/internal/SynapseTargetVisitor.h"

namespace marocco {
namespace routing {
namespace internal {

using namespace euter;

SynapseTargetVisitor::return_type SynapseTargetVisitor::generate_n_targets(size_t n)
{
	return_type rv;
	rv.reserve(n);
	for (size_t ii = 0; ii < n; ++ii) {
		rv.push_back(SynapseType(ii));
	}
	return rv;
}

// multi conductance AdEx
SynapseTargetVisitor::return_type SynapseTargetVisitor::
operator()(cell_t<CellType::EIF_multicond_exp_isfa_ista> const& v, size_t nrn) const
{
	auto const& cellparams = v.parameters()[nrn];
	const size_t num_tau_syn = cellparams.tau_syn.size();
	const size_t num_E_rev = cellparams.e_rev.size();

	if (num_tau_syn != num_E_rev)
		throw std::runtime_error(
			"cell parameters tau_syn and e_rev have different sizes, which is not allowed!");

	return generate_n_targets(num_tau_syn);
}

// multi conductance LIF
SynapseTargetVisitor::return_type SynapseTargetVisitor::
operator()(cell_t<CellType::IF_multicond_exp> const& v, size_t nrn) const
{

	auto const& cellparams = v.parameters()[nrn];
	const size_t num_tau_syn = cellparams.tau_syn.size();
	const size_t num_E_rev = cellparams.e_rev.size();

	if (num_tau_syn != num_E_rev)
		throw std::runtime_error(
			"cell parameters tau_syn and e_rev have different sizes, which is not allowed!");

	return generate_n_targets(num_tau_syn);
}

} // namespace internal
} // namespace routing
} // namespace marocco
