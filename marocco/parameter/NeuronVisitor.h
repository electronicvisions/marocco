#pragma once
// neuron analog parameter transformation for the HMF System

#include <type_traits>
#include <stdexcept>
#include <numeric>
#include <vector>
#include <array>

#include "euter/typedcellparametervector.h"
#include "calibtic/HMF/NeuronCollection.h"

#include "marocco/config.h"
#include "marocco/graph.h"

#include "marocco/parameter/detail.h"

#include "marocco/routing/results/SynapticInputs.h"

namespace marocco {
namespace parameter {

struct NeuronSharedParameterRequirements
{
	typedef void return_type;
	typedef HMF::Coordinate::NeuronOnHICANN neuron_t;
	typedef HMF::Coordinate::FGBlockOnHICANN group_t;

	template <euter::CellType N>
	using cell_t = euter::TypedCellParameterVector<N>;

	template <euter::CellType N>
	typename std::enable_if<!detail::has_v_reset<cell_t<N>>::value, return_type>::type
	operator() (cell_t<N> const& /*unused*/, size_t /*unused*/, neuron_t const& /*unused*/)
	{
		std::stringstream ss;
		ss << "unsupported cell type: " << euter::getCellTypeName(N);
		throw std::runtime_error(ss.str());
	}

	// collect all V_reset values per _shared_ FG block, i.e.
	// for each HICANN half, the left V_reset value is connected to all even
	// numbered denmems, and the right V_reset to the odd numbered.
	template <euter::CellType N>
	typename std::enable_if<detail::has_v_reset<cell_t<N>>::value, return_type>::type
	operator() (cell_t<N> const& v, size_t neuron_bio_id, neuron_t const& n)
	{
		auto const& cellparams = v.parameters()[neuron_bio_id];
		mVResets.insert(cellparams.v_reset);
		mVResetValues[n.toSharedFGBlockOnHICANN().toEnum()].push_back(cellparams.v_reset);
	}

	// get the mean v_reset value per FG block
	double get_mean_v_reset(group_t const& g) const
	{
		auto const& vals = mVResetValues[g.toEnum()];
		return std::accumulate(vals.begin(), vals.end(), 0.0) / vals.size();
	}

	// get mean v_reset over all neurons
	double get_mean_v_reset() const
	{
		double sum = 0;
		size_t cnt = 0;

		for(auto const& vals : mVResetValues) {
			sum += std::accumulate(vals.begin(), vals.end(), 0.0);
			cnt += vals.size();
		}

		double const mean = cnt != 0 ? sum/cnt : 0;

		return mean;
	}

	// get all different v_reset values
	std::set<double> get_v_resets() const {
		return mVResets;
	}

private:
	std::array<std::vector<double>, group_t::enum_type::end> mVResetValues;
	std::set<double> mVResets;
};

struct TransformNeurons
{
	typedef void return_type;
	typedef size_t size_type;
	typedef sthal::HICANN chip_t;
	typedef HMF::NeuronCollection calib_t;
	typedef HMF::Coordinate::NeuronOnHICANN neuron_t;
	typedef routing::results::SynapticInputs synapse_targets_t;

	template <euter::CellType N>
		using cell_t = euter::TypedCellParameterVector<N>;

	TransformNeurons(double alphaV, double shiftV) : mAlphaV(alphaV), mShiftV(shiftV) {}

	template <euter::CellType N>
	return_type operator()(
		cell_t<N> const& /*unused*/,
		calib_t const& /*unused*/,
		size_t /*unused*/,
		neuron_t const& /*unused*/,
		synapse_targets_t const& /*unused*/,
		chip_t& /*unused*/) const
	{
		std::stringstream ss;
		ss << "unsupported cell type: " << euter::getCellTypeName(N);
		throw std::runtime_error(ss.str());
	}

	/// AdEx Parameter Transformation
	return_type operator()(
		cell_t<euter::CellType::EIF_cond_exp_isfa_ista> const& v,
		calib_t const& calib,
		size_t neuron_bio_id,
		neuron_t const& neuron_hw_id,
		synapse_targets_t const& synaptic_targets,
		chip_t& chip) const;

	/// LIF Parameter Transformation
	return_type operator()(
		cell_t<euter::CellType::IF_cond_exp> const& v,
		calib_t const& calib,
		size_t neuron_bio_id,
		neuron_t const& neuron_hw_id,
		synapse_targets_t const& synaptic_targets,
		chip_t& chip) const;

	/// AdEx with multiple time constants Parameter Transformation
	return_type operator()(
		cell_t<euter::CellType::EIF_multicond_exp_isfa_ista> const& v,
		calib_t const& calib,
		size_t neuron_bio_id,
		neuron_t const& neuron_hw_id,
		synapse_targets_t const& synaptic_targets,
		chip_t& chip) const;

	/// LIF with multiple time constants Parameter Transformation
	return_type operator()(
		cell_t<euter::CellType::IF_multicond_exp> const& v,
		calib_t const& calib,
		size_t neuron_bio_id,
		neuron_t const& neuron_hw_id,
		synapse_targets_t const& synaptic_targets,
		chip_t& chip) const;

private:
	static void
	assert_synapse_target_mapping_is_default(synapse_targets_t::value_type const& targets);

	double mAlphaV; //!< unitless scaling factor
	double mShiftV; //!< voltage shift in Volt

};


void transform_analog_neuron(
	HMF::NeuronCollection const& calib,
	euter::Population const& pop,
	size_t neuron_bio_id,
	HMF::Coordinate::NeuronOnHICANN const& neuron_hw_id,
	routing::results::SynapticInputs const& synapse_targets,
	TransformNeurons& visitor,
	sthal::HICANN& chip);

} // namespace parameter
} // namespace marocco
