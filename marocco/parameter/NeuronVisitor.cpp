#include "marocco/parameter/NeuronVisitor.h"
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
	chip_type<hardware_system_t>::type const& chip,
	double const bio_v_reset, ///< pyNN v_reset in mV
	double const target_v_reset, ///< target HW v_reset in Volt
	HMF::Coordinate::NeuronOnHICANN const& neuron_hw_id)
{
	HMF::NeuronCalibrationParameters params;

	params.bigcap = chip.neurons.config.bigcap[neuron_hw_id.y()];
	params.hw_neuron_size = 1; // always use neuron size = 1, cf. #1559
	// compute shiftV so that v_reset is transformed to target_v_reset (#1693)
	static const double mV_to_V = 0.001;
	params.shiftV = target_v_reset - bio_v_reset*mV_to_V*params.alphaV;

	return params;
}

void transform_analog_neuron(
	TransformNeurons::calib_t  const& calib,
	Population const& pop,
	size_t const neuron_bio_id,
	NeuronOnHICANN const& hw_neuron_id,
	TransformNeurons& visitor,
	chip_type<hardware_system_t>::type& chip)
{
	// configure analog neuron parameters
	visitCellParameterVector(
		pop.parameters(),
		visitor,

		calib,
		neuron_bio_id,
		hw_neuron_id,
		chip);
}


// AdEx Parameter Transformation
typename TransformNeurons::return_type
TransformNeurons::operator() (
	cell_t<CellType::EIF_cond_exp_isfa_ista> const& v,
	calib_t const& calib,
	size_t const neuron_bio_id,
	neuron_t const& neuron_hw_id,
	chip_t& chip) const
{
	MAROCCO_DEBUG("parameter transformation of AdEx neuron: " << neuron_bio_id);

	auto const& cellparams = v.parameters()[neuron_bio_id];

	auto const params = calibration_parameters_for_neuron(
		chip, cellparams.v_reset, mTargetVReset, neuron_hw_id);

	auto hwparams = calib.applyNeuronCalibration(
		cellparams, neuron_hw_id.id(), params);

	hwparams.toHW(neuron_hw_id, chip.floating_gates);
}


// LIF Parameter Transformation
typename TransformNeurons::return_type
TransformNeurons::operator() (
	cell_t<CellType::IF_cond_exp> const& v,
	calib_t const& calib,
	size_t const neuron_bio_id,
	neuron_t const& neuron_hw_id,
	chip_t& chip) const
{
	MAROCCO_DEBUG("parameter transformation of LIF neuron: " << neuron_bio_id);

	auto const& cellparams = v.parameters()[neuron_bio_id];

	auto const params = calibration_parameters_for_neuron(
		chip, cellparams.v_reset, mTargetVReset, neuron_hw_id);

	auto hwparams = calib.applyNeuronCalibration(
		cellparams, neuron_hw_id.id(), params);

	hwparams.toHW(neuron_hw_id, chip.floating_gates);
}

} // namespace parameter
} // namespace marocco
