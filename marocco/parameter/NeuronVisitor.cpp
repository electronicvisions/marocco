#include "marocco/parameter/NeuronVisitor.h"

#include <tuple>

#include "marocco/Logger.h"
#include "hal/Coordinate/iter_all.h"

using namespace HMF::Coordinate;

template<typename T,
	T Max = std::numeric_limits<T>::max(),
	T Min = std::numeric_limits<T>::min()>
using iclip = rant::integral_range<T, Max, Min,
	rant::clip_on_error<T,
		std::integral_constant<T, Max>,
		std::integral_constant<T, Min>>>;

typedef iclip<unsigned, 1023> fg_clip;


namespace marocco {
namespace parameter {

inline
fg_clip current2fg(double current_in_nA)
{
	double const max_fg_current_in_nA = 2500.;
	return current_in_nA / max_fg_current_in_nA * std::numeric_limits<fg_clip>::max();
}

inline
fg_clip voltage2fg(double voltage_in_mV)
{
	double const max_fg_voltage_in_mV = 1800.;
	return voltage_in_mV / max_fg_voltage_in_mV * std::numeric_limits<fg_clip>::max();
}

HMF::NeuronCalibrationParameters calibration_parameters_for_neuron(
	sthal::HICANN const& chip,
	double const alphaV,
	double const shiftV,
	HMF::Coordinate::NeuronOnHICANN const& neuron_hw_id)
{
	HMF::NeuronCalibrationParameters params;

	params.bigcap = chip.neurons.config.bigcap[neuron_hw_id.y()];
	params.hw_neuron_size = 1; // always use neuron size = 1, cf. #1559

	params.shiftV = shiftV;
	params.alphaV = alphaV;

	return params;
}

void transform_analog_neuron(
	TransformNeurons::calib_t const& calib,
	Population const& pop,
	size_t const neuron_bio_id,
	NeuronOnHICANN const& hw_neuron_id,
	routing::results::SynapticInputs const& synapse_targets,
	TransformNeurons& visitor,
	sthal::HICANN& chip)
{
	// configure analog neuron parameters
	visitCellParameterVector(
		pop.parameters(), visitor, calib, neuron_bio_id, hw_neuron_id, synapse_targets, chip);
}

void TransformNeurons::assert_synapse_target_mapping_is_default(
	synapse_targets_t::value_type const& targets)
{
	static const synapse_targets_t::value_type default_mapping =
		{SynapseType::excitatory, SynapseType::inhibitory};
	assert(targets == default_mapping);
	std::ignore = targets;
	std::ignore = default_mapping;
}

// AdEx Parameter Transformation
typename TransformNeurons::return_type TransformNeurons::operator()(
	cell_t<CellType::EIF_cond_exp_isfa_ista> const& v,
	calib_t const& calib,
	size_t const neuron_bio_id,
	neuron_t const& neuron_hw_id,
	synapse_targets_t const& synaptic_targets,
	chip_t& chip) const
{
	MAROCCO_TRACE("parameter transformation of AdEx neuron: " << neuron_bio_id);

	assert_synapse_target_mapping_is_default(synaptic_targets[neuron_hw_id]);

	auto const& cellparams = v.parameters()[neuron_bio_id];

	auto const params =
	    calibration_parameters_for_neuron(chip, mAlphaV, mShiftV, neuron_hw_id);

	auto hwparams = calib.applyNeuronCalibration(
		cellparams, neuron_hw_id.id(), params);

	hwparams.toHW(neuron_hw_id, chip.floating_gates);
}


// LIF Parameter Transformation
typename TransformNeurons::return_type TransformNeurons::operator()(
	cell_t<CellType::IF_cond_exp> const& v,
	calib_t const& calib,
	size_t const neuron_bio_id,
	neuron_t const& neuron_hw_id,
	synapse_targets_t const& synaptic_targets,
	chip_t& chip) const
{
	MAROCCO_TRACE("parameter transformation of LIF neuron: " << neuron_bio_id);

	assert_synapse_target_mapping_is_default(synaptic_targets[neuron_hw_id]);

	auto const& cellparams = v.parameters()[neuron_bio_id];

	auto const params =
	    calibration_parameters_for_neuron(chip, mAlphaV, mShiftV, neuron_hw_id);

	auto hwparams = calib.applyNeuronCalibration(
		cellparams, neuron_hw_id.id(), params);

	hwparams.toHW(neuron_hw_id, chip.floating_gates);
}

#define COPY_MEMBER(lhs, rhs, member) lhs.member = rhs.member

PyNNParameters::IF_cond_exp to_IF_cond_exp(
	PyNNParameters::IF_multicond_exp const& multicond_exp, size_t excitatory, size_t inhibitory)
{
	PyNNParameters::IF_cond_exp cond_exp;

	COPY_MEMBER(cond_exp, multicond_exp, tau_refrac);
	COPY_MEMBER(cond_exp, multicond_exp, cm);
	COPY_MEMBER(cond_exp, multicond_exp, v_rest);
	COPY_MEMBER(cond_exp, multicond_exp, tau_m);
	COPY_MEMBER(cond_exp, multicond_exp, i_offset);
	COPY_MEMBER(cond_exp, multicond_exp, v_thresh);
	COPY_MEMBER(cond_exp, multicond_exp, v_reset);
	COPY_MEMBER(cond_exp, multicond_exp, v);

	COPY_MEMBER(cond_exp, multicond_exp, record_spikes);
	COPY_MEMBER(cond_exp, multicond_exp, record_v);
	COPY_MEMBER(cond_exp, multicond_exp, record_gsyn);

	assert(multicond_exp.tau_syn.size() == multicond_exp.e_rev.size());

	assert(excitatory < multicond_exp.tau_syn.size());
	cond_exp.tau_syn_E = multicond_exp.tau_syn[excitatory];
	cond_exp.e_rev_E = multicond_exp.e_rev[excitatory];

	assert(inhibitory < multicond_exp.tau_syn.size());
	cond_exp.tau_syn_I = multicond_exp.tau_syn[inhibitory];
	cond_exp.e_rev_I = multicond_exp.e_rev[inhibitory];

	return cond_exp;
}

PyNNParameters::EIF_cond_exp_isfa_ista to_EIF_cond_exp_isfa_ista(
	PyNNParameters::EIF_multicond_exp_isfa_ista const& multicond_exp,
	size_t excitatory,
	size_t inhibitory)
{
	PyNNParameters::EIF_cond_exp_isfa_ista cond_exp;

	COPY_MEMBER(cond_exp, multicond_exp, tau_refrac);
	COPY_MEMBER(cond_exp, multicond_exp, a);
	COPY_MEMBER(cond_exp, multicond_exp, cm);
	COPY_MEMBER(cond_exp, multicond_exp, delta_T);
	COPY_MEMBER(cond_exp, multicond_exp, tau_m);
	COPY_MEMBER(cond_exp, multicond_exp, i_offset);
	COPY_MEMBER(cond_exp, multicond_exp, v_thresh);
	COPY_MEMBER(cond_exp, multicond_exp, b);
	COPY_MEMBER(cond_exp, multicond_exp, v_reset);
	COPY_MEMBER(cond_exp, multicond_exp, v_spike);
	COPY_MEMBER(cond_exp, multicond_exp, v);
	COPY_MEMBER(cond_exp, multicond_exp, tau_w);
	COPY_MEMBER(cond_exp, multicond_exp, w);
	COPY_MEMBER(cond_exp, multicond_exp, v_rest);

	COPY_MEMBER(cond_exp, multicond_exp, record_spikes);
	COPY_MEMBER(cond_exp, multicond_exp, record_v);
	COPY_MEMBER(cond_exp, multicond_exp, record_w);
	COPY_MEMBER(cond_exp, multicond_exp, record_gsyn);

	assert(multicond_exp.tau_syn.size() == multicond_exp.e_rev.size());

	assert(excitatory < multicond_exp.tau_syn.size());
	cond_exp.tau_syn_E = multicond_exp.tau_syn[excitatory];
	cond_exp.e_rev_E = multicond_exp.e_rev[excitatory];

	assert(inhibitory < multicond_exp.tau_syn.size());
	cond_exp.tau_syn_I = multicond_exp.tau_syn[inhibitory];
	cond_exp.e_rev_I = multicond_exp.e_rev[inhibitory];

	return cond_exp;
}
#undef COPY_MEMBER

// multi time constant AdEx Parameter Transformation
typename TransformNeurons::return_type TransformNeurons::operator()(
	cell_t<CellType::EIF_multicond_exp_isfa_ista> const& v,
	calib_t const& calib,
	size_t const neuron_bio_id,
	neuron_t const& neuron_hw_id,
	synapse_targets_t const& synaptic_targets,
	chip_t& chip) const
{
	MAROCCO_TRACE("parameter transformation of multicond AdEx neuron: " << neuron_bio_id);

	auto const& synapse_types = synaptic_targets[neuron_hw_id];

	auto const& cellparams = to_EIF_cond_exp_isfa_ista(
		v.parameters()[neuron_bio_id], int(synapse_types[left]), int(synapse_types[right]));

	auto const params =
	    calibration_parameters_for_neuron(chip, mAlphaV, mShiftV, neuron_hw_id);

	auto hwparams = calib.applyNeuronCalibration(cellparams, neuron_hw_id.id(), params);

	hwparams.toHW(neuron_hw_id, chip.floating_gates);
}

// multi time constant LIF Parameter Transformation
typename TransformNeurons::return_type TransformNeurons::operator()(
	cell_t<CellType::IF_multicond_exp> const& v,
	calib_t const& calib,
	size_t const neuron_bio_id,
	neuron_t const& neuron_hw_id,
	synapse_targets_t const& synaptic_targets,
	chip_t& chip) const
{
	MAROCCO_TRACE("parameter transformation of multicond LIF neuron: " << neuron_bio_id);

	auto const& synapse_types = synaptic_targets[neuron_hw_id];

	auto const& cellparams = to_IF_cond_exp(
		v.parameters()[neuron_bio_id], int(synapse_types[left]), int(synapse_types[right]));

	auto const params =
	    calibration_parameters_for_neuron(chip, mAlphaV, mShiftV, neuron_hw_id);

	auto hwparams = calib.applyNeuronCalibration(cellparams, neuron_hw_id.id(), params);

	hwparams.toHW(neuron_hw_id, chip.floating_gates);
}


} // namespace parameter
} // namespace marocco
