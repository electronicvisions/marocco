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

	template <euter::CellType N>
		using cell_t = euter::TypedCellParameterVector<N>;

	template <euter::CellType N>
	return_type operator() (cell_t<N> const& /*cell*/, size_t /*nrn*/) const
	{
		throw std::runtime_error("unsupported cell type");
	}

	// AdEx Parameter Transformation
	return_type operator() (cell_t<euter::CellType::EIF_cond_exp_isfa_ista> const& v, size_t nrn) const
	{
		auto const& cellparams = v.parameters()[nrn];
		return cellparams.cm;
	}

	// LIF Parameter Transformation
	return_type operator() (cell_t<euter::CellType::IF_cond_exp> const& v, size_t nrn) const
	{
		auto const& cellparams = v.parameters()[nrn];
		return cellparams.cm;
	}

	/// AdEx with multiple time constants Parameter Transformation
	return_type operator()(cell_t<euter::CellType::EIF_multicond_exp_isfa_ista> const& v, size_t nrn) const
	{
		auto const& cellparams = v.parameters()[nrn];
		return cellparams.cm;
	}

	/// LIF with multiple time constants Parameter Transformation
	return_type operator()(cell_t<euter::CellType::IF_multicond_exp> const& v, size_t nrn) const
	{
		auto const& cellparams = v.parameters()[nrn];
		return cellparams.cm;
	}
};

} // namespace parameter
} // namespace marocco
