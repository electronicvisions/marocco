#pragma once

#include <boost/serialization/nvp.hpp>

namespace pymarocco {

/// High-level Access to parameters controlling the input placement algorithm
class InputPlacement
{
public:
	InputPlacement();

	/// true: consider firing rate of spike sources and available input bandwidth
	/// false: don't consider bandwidth
	/// default: false
	bool consider_rate;

	/// utilization of available bandwidth
	/// range: [0,1]
	/// default: 1.
	double bandwidth_utilization;

private:

	friend class boost::serialization::access;
	template <typename Archive>
	void serialize(Archive& ar, unsigned int const)
	{
		using boost::serialization::make_nvp;
		ar& make_nvp("consider_rate", consider_rate)
		  & make_nvp("bandwidth_utilization", bandwidth_utilization);
	}
};

} // pymarocco
