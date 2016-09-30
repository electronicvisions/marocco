#pragma once

#include <boost/serialization/nvp.hpp>

namespace pymarocco {

/// High-level Access to parameters controlling the transformation of neuron and synapse models onto the hardware
class ParamTrafo
{
public:
	ParamTrafo();

	// Neurons

	/// Choose whether the small or big hardware capacitors shall be used for the hardware neurons.
	/// default: false 
	/// FIXME: This might be changed to true. Currently set to false, to be compatible with previously hard-coded
	/// use of small capacitors in parameter transformation.
	/// @note: in the future, the choice of the capacitor should be done by the parameter transformation algorithm,
	///        so that the parameters are optimally mapped to the hardware, depending on the available calibrations
	bool use_big_capacitors;

	/// voltage scaling factor for potentials.
	/// default: 10.
	double alpha_v;

	/// voltage shift factor for potentials in V
	/// default: 1.2.
	double shift_v;

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
