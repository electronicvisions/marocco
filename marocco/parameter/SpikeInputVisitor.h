#pragma once
// neuron analog parameter transformation for the HMF System

#include <type_traits>
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
	typedef size_t size_type;
	typedef chip_type<hardware_system_t>::type chip_t;

	typedef std::vector<sthal::Spike> SpikeList;

	template <CellType N>
	using cell_t = TypedCellParameterVector<N>;

	SpikeInputVisitor(
			pymarocco::PyMarocco const& pymarocco,
			SpikeList& spikes, int seed,
			double experiment_duration //!< PyNN experiment duration in ms
			);

	template <CellType N>
	return_type operator() (
		cell_t<N> const& /*unused*/,
		HMF::HICANN::L1Address const& /*unused*/,
		size_t /*unused*/,
		chip_t& /*unused*/)
	{
		std::stringstream ss;
		ss << "unsupported spike input " << getCellTypeName(N);
		throw std::runtime_error(ss.str());
	}

	// Spike Source Array Transformation
	return_type operator() (
		cell_t<CellType::SpikeSourceArray> const& v,
		HMF::HICANN::L1Address const& l1,
		size_t neuron_id,
		chip_t& chip);

	// Spike Source Poisson Transformation
	return_type operator() (
		cell_t<CellType::SpikeSourcePoisson> const& v,
		HMF::HICANN::L1Address const& l1,
		size_t neuron_id,
		chip_t& chip);

	pymarocco::PyMarocco const& mPyMarocco;
	SpikeList& mSpikes;
	std::mt19937 mRNG;
	double const mExperimentDuration; //!< PyNN experiment duration in ms
};


void transform_input_spikes(
	Population const& pop,
	HMF::HICANN::L1Address const& l1,
	size_t neuron_id,
	chip_type<hardware_system_t>::type& chip,
	SpikeInputVisitor& visitor);

} // namespace parameter
} // namespace marocco
