#pragma once

#include <type_traits>
#include <stdexcept>

#include "euter/typedcellparametervector.h"

namespace marocco {
namespace parameter {

/// access to biological membrane capacitance, needed for synapse trafo
struct CMVisitor
{
	typedef double return_type;

	template <CellType N>
		using cell_t = TypedCellParameterVector<N>;

	template <CellType N>
	return_type operator() (cell_t<N> const& /*cell*/, size_t /*nrn*/) const
	{
		throw std::runtime_error("unsupported cell type");
	}

	// AdEx Parameter Transformation
	return_type operator() (cell_t<CellType::EIF_cond_exp_isfa_ista> const& v, size_t nrn) const
	{
		auto const& cellparams = v.parameters()[nrn];
		return cellparams.cm;
	}

	// LIF Parameter Transformation
	return_type operator() (cell_t<CellType::IF_cond_exp> const& v, size_t nrn) const
	{
		auto const& cellparams = v.parameters()[nrn];
		return cellparams.cm;
	}
};

} // namespace parameter
} // namespace marocco
