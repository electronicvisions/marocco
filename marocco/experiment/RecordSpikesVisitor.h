#pragma once

#include <type_traits>

#include "euter/typedcellparametervector.h"

#include "marocco/parameter/detail.h"

namespace marocco {
namespace experiment {

struct RecordSpikesVisitor
{
	typedef bool return_type;

	template <CellType N>
	using cell_type = TypedCellParameterVector<N>;
	template <typename T>
	using has_record_spikes = typename parameter::detail::has_record_spikes<T>;

	// Overload for neurons which support recording of spikes
	template <CellType N>
	typename std::enable_if<has_record_spikes<cell_type<N> >::value, return_type>::type operator()(
		cell_type<N> const& v, size_t const neuron_index) const
	{
		auto const& cell = v.parameters()[neuron_index];
		return cell.record_spikes;
	}

	// Overload for neurons which do not support recording of spikes
	template <CellType N>
	typename std::enable_if<!has_record_spikes<cell_type<N> >::value, return_type>::type operator()(
		cell_type<N> const& /* v */, size_t const /* neuron_index */) const
	{
		return false;
	}
}; // RecordSpikesVisitor

bool record_spikes(CellParameterVector const& parameters, size_t const neuron_index)
{
	RecordSpikesVisitor visitor;
	return visitCellParameterVector(parameters, visitor, neuron_index);
}

} // namespace experiment
} // namespace marocco
