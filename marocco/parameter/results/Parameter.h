#pragma once

#ifndef PYPLUSPLUS
#include <unordered_map>
#endif // !PYPLUSPLUS

#include "halco/hicann/v2/synapse.h"

#include <boost/serialization/export.hpp>

namespace boost {
namespace serialization {
class access;
} // namespace serialization
} // namespace boost

namespace marocco {
namespace parameter {
namespace results {

/**
 * @brief Stores distorted values of input parameters, e.g. synaptic weight.
 * @todo Add more parameter, e.g. clipped time constants.
 */
class Parameter
{
public:
	typedef halco::hicann::v2::SynapseOnWafer hardware_synapse_type;

	// bio weight in uS
	typedef double weight_type;

	void set_weight(
	    hardware_synapse_type const& hardware_synapse, weight_type const distorted_weight);

	// weight in bio uS
	weight_type get_weight(hardware_synapse_type const& hardware_synapse) const;

private:
#ifndef PYPLUSPLUS
	std::unordered_map<hardware_synapse_type, weight_type> m_weights;
#endif
	friend class boost::serialization::access;
	template <typename Archiver>
	void serialize(Archiver& ar, const unsigned int /* version */);
};

} // results
} // parameter
} // marocco

BOOST_CLASS_EXPORT_KEY(::marocco::parameter::results::Parameter)
