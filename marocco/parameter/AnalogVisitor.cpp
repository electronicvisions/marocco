#include "marocco/parameter/AnalogVisitor.h"

using namespace HMF::Coordinate;

namespace marocco {
namespace parameter {

void transform_analog_outputs(
	AnalogVisitor::calib_t const& calib,
	Population const& pop,
	size_t const neuron_bio_id,
	NeuronOnHICANN const& nrn,
	AnalogVisitor& visitor,
	chip_type<hardware_system_t>::type& chip)
{
	visitCellParameterVector(
		pop.parameters(),
		visitor,

		calib,
		neuron_bio_id,
		nrn,
		chip);
}

void AnalogVisitor::set_analog(
	chip_t& chip,
	calib_t const& calib,
	HMF::Coordinate::AnalogOnHICANN const& aout,
	HMF::Coordinate::NeuronOnHICANN const& nrn) const
{
	// enable
	chip.analog.enable(aout);

	// configure
	//debug(this) << "setting Analog" << N << " neuron: " << neurons_hw_offset
		//<< " top: "  << is_top(neurons_hw_offset)
		//<< " even: " << is_even(neurons_hw_offset) << " to ";

	auto& analog = chip.analog;
	if (is_top(nrn) && is_even(nrn)) {
		analog.set_membrane_top_even(aout);
	} else if (is_top(nrn) && !is_even(nrn)) {
		analog.set_membrane_top_odd(aout);
	} else if (is_bottom(nrn) && is_even(nrn)) {
		analog.set_membrane_bot_even(aout);
	} else { // (is_bottom(nrn) && !is_even(nrn))
		analog.set_membrane_bot_odd(aout);
	}

	// FIXME: need access to inputs first
	//check(analog.input);
}


template <CellType N>
typename std::enable_if<detail::has_record_v<AnalogVisitor::cell_t<N>>::value,
                        AnalogVisitor::return_type>::type
AnalogVisitor::operator() (
	cell_t<N> const& v,
	calib_t const& calib,
	size_t const neuron_bio_id,
	HMF::Coordinate::NeuronOnHICANN const& nrn,
	chip_t& chip)
{
	using namespace HMF::Coordinate;

	auto const& cell = v.parameters()[neuron_bio_id];
	if (cell.record_v)
	{

		size_t const aout = countActiveAouts();
		//if (aout >= 2)
		//
		MAROCCO_DEBUG("AnalogOut neuron " << chip.index() << " " << nrn << " bio: " << neuron_bio_id << " marked for aout " << aout);

		if (aout >= 1) // hack
		{
			error(this) << "two aouts mapped already,"
			   " cannot output another neuron";
			return;
		}

		// remember mapping
		mMapping[aout] = nrn;

		set_analog(chip, calib, AnalogOnHICANN(aout), nrn);

		// activate analog out @ neuron
		chip.neurons[nrn].enable_aout(true);

		MAROCCO_INFO("Analog output: " << chip.index() << " " << nrn << " " << AnalogOnHICANN(aout));
		chip.enable_aout(nrn, AnalogOnHICANN(aout));
	}
}

template <CellType N>
typename std::enable_if<!detail::has_record_v<AnalogVisitor::cell_t<N>>::value,
                        AnalogVisitor::return_type>::type
AnalogVisitor::operator() (
	cell_t<N> const& /*unused*/,
	calib_t const& /*unused*/,
	size_t const /*unused*/,
	HMF::Coordinate::NeuronOnHICANN const& /*unused*/,
	chip_t& /*unused*/)
{
	// do nothing;
	MAROCCO_WARN("celltype " << (int)N << "doesn't support record_v");
}

} // namespace parameter
} // namespace marocco
