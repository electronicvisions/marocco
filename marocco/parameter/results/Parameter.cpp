#include "marocco/parameter/results/Parameter.h"

#include <boost/serialization/unordered_map.hpp>

namespace marocco {
namespace parameter {
namespace results {

void Parameter::set_weight(
    hardware_synapse_type const& hardware_synapse, weight_type const distorted_weight)
{
	m_weights[hardware_synapse] = distorted_weight;
}

Parameter::weight_type Parameter::get_weight(hardware_synapse_type const& hardware_synapse) const
{
	return m_weights.at(hardware_synapse);
}

template <typename Archiver>
void Parameter::serialize(Archiver& ar, const unsigned int /* version */)
{
	using namespace boost::serialization;
	// clang-format off
	ar & make_nvp("distorted_weights", m_weights);
	// clang-format on
}

} // results
} // parameter
} // marocco

BOOST_CLASS_EXPORT_IMPLEMENT(::marocco::parameter::results::Parameter)

#include "boost/serialization/serialization_helper.tcc"
EXPLICIT_INSTANTIATE_BOOST_SERIALIZE(::marocco::parameter::results::Parameter)
