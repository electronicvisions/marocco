#pragma once

#include <type_traits>
#include <stdexcept>

#include "euter/typedcellparametervector.h"

namespace marocco {
namespace placement {
namespace internal {

/**
 * Extract the expected average firing rate of spike sources.
 *
 * Returns the exptected mean firing rate in Hz in hardware time, depending on
 * the parameters of the spike source, and taking into account the speedup
 * factor.
 *
 * For a \c SpikeSourcePoisson, the parameter `rate` is used.
 * For a \c SpikeSourceArray, the mean rate is calculated from spike times
 * given by parameter `spike_times`:
 *   0 entries: the mean rate is 0 Hz.
 *   1 entry: the assumed biological mean rate is 1 Hz.
 *   > 1 entries: the mean rate between the first and last spike time is used: 
 *                mean rate = (Nr of spikes - 1)/(t_last-t_first)
 */
struct FiringRateVisitor
{
	typedef double return_type;

	template <CellType N>
	using cell_t = TypedCellParameterVector<N>;

	FiringRateVisitor(double speedup);

	template <CellType N>
	return_type operator() (
		cell_t<N> const& /*unused*/,
		size_t /*unused*/)
	{
		std::stringstream ss;
		ss << "unsupported spike input " << getCellTypeName(N);
		throw std::runtime_error(ss.str());
	}

	// Spike Source Array
	return_type operator() (
		cell_t<CellType::SpikeSourceArray> const& v,
		size_t neuron_id);

	// Spike Source Poisson
	return_type operator() (
		cell_t<CellType::SpikeSourcePoisson> const& v,
		size_t neuron_id);

	double const m_speedup;
};

} // namespace internal
} // namespace placement
} // namespace marocco
