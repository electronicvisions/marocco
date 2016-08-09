#pragma once
// neuron analog parameter transformation for the HMF System

#include <sstream>
#include <stdexcept>
#include <random>

#include "euter/typedcellparametervector.h"

#include "marocco/config.h"
#include "marocco/graph.h"
#include "pymarocco/PyMarocco.h"

namespace marocco {
namespace parameter {

struct SpikeInputVisitor
{
	typedef void return_type;
	typedef std::vector<double> spikes_type;

	template <CellType N>
	using cell_t = TypedCellParameterVector<N>;

	template <CellType N>
	void operator()(cell_t<N> const&, size_t const, size_t const, double const, spikes_type&) const
	{
		std::stringstream ss;
		ss << "unsupported spike input " << getCellTypeName(N);
		throw std::runtime_error(ss.str());
	}

	void operator()(
		cell_t<CellType::SpikeSourceArray> const& v,
		size_t const neuron_id,
		size_t const seed,
		double const experiment_duration,
		spikes_type& spikes) const;

	void operator()(
		cell_t<CellType::SpikeSourcePoisson> const& v,
		size_t const neuron_id,
		size_t const seed,
		double const experiment_duration,
		spikes_type& spikes) const;
};

/**
 * @brief Extract or calculate spike times from cell parameters.
 * @param experiment_duration PyNN experiment duration in ms
 */
SpikeInputVisitor::spikes_type extract_input_spikes(
	Population const& pop,
	size_t const neuron_id,
	size_t const seed,
	double const experiment_duration);

} // namespace parameter
} // namespace marocco
