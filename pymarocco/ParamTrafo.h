#pragma once

#include <boost/serialization/nvp.hpp>
#include "sthal/SpeedUp.h"

namespace pymarocco {

/// High-level Access to parameters controlling the transformation of neuron and synapse models onto the hardware
class ParamTrafo
{
public:
	ParamTrafo();

	// Neurons

	/// Choose whether the small or big hardware capacitors shall be used for the hardware neurons.
	/// default: true
	/// @note: in the future, the choice of the capacitor should be done by the parameter transformation algorithm,
	///        so that the parameters are optimally mapped to the hardware, depending on the available calibrations
	bool use_big_capacitors;

	/// Choose whether the SLOW, NORMAL or FAST i_gl setting should be used.
	/// default: NORMAL
	sthal::SpeedUp i_gl_speedup;

	/// voltage scaling factor for potentials.
	/// default: 10.
	double alpha_v;

	/// voltage shift factor for potentials in V
	/// default: 1.2.
	double shift_v;

	/// sets shift and scaling factor so that the largest (smallest) used bio voltage
        /// max_experiment_voltage (min_experiment_voltage) is represented by the maximum (minimum)
        /// hardware voltage max_hw_voltage (min_hw_voltage).
        /// Maximum and minimum is normally given by the reversal potentials.
        /// Values should be chosen which allow the membrane to operate in region b/w 0.3 V and 1 V.
        /// Caveat: calibration shows a saturation above 1.4 V for the exc reversal potential
	void set_trafo_with_bio_to_hw_ratio(
	    double const max_experiment_voltage,
	    double const min_experiment_voltage,
	    double const max_hw_voltage = 1.4,
	    double const min_hw_voltage = 0.3);

	/// Helper to check transformation
	double calculate_hw_value(double const bio_value);

private:

	friend class boost::serialization::access;
	template <typename Archive>
	void serialize(Archive& ar, unsigned int const)
	{
		using boost::serialization::make_nvp;
		ar& make_nvp("use_big_capacitors", use_big_capacitors)
		  & make_nvp("alpha_v", alpha_v)
		  & make_nvp("shift_v", shift_v);
	}
};

} // pymarocco
