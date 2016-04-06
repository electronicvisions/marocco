#include "marocco/placement/parameters/InputPlacement.h"

#include <stdexcept>
#include <boost/serialization/nvp.hpp>

namespace marocco {
namespace placement {
namespace parameters {

InputPlacement::InputPlacement()
	: m_consider_firing_rate(false), m_bandwidth_utilization(1.0)
{
}

void InputPlacement::consider_firing_rate(bool enable)
{
	m_consider_firing_rate = enable;
}

bool InputPlacement::consider_firing_rate() const
{
	return m_consider_firing_rate;
}

void InputPlacement::bandwidth_utilization(double fraction)
{
	if (fraction < 0.0 || fraction > 1.0) {
		throw std::invalid_argument("bandwidth utilization fraction has to be in range [0.0, 1.0]");
	}
	m_bandwidth_utilization = fraction;
}

double InputPlacement::bandwidth_utilization() const
{
	return m_bandwidth_utilization;
}


template <typename Archive>
void InputPlacement::serialize(Archive& ar, unsigned int const /* version */)
{
	using namespace boost::serialization;
	// clang-format off
	ar & make_nvp("consider_firing_rate", m_consider_firing_rate)
	   & make_nvp("bandwidth_utilization", m_bandwidth_utilization);
	// clang-format on
}

} // namespace parameters
} // namespace placement
} // namespace marocco

BOOST_CLASS_EXPORT_IMPLEMENT(::marocco::placement::parameters::InputPlacement)

#include "boost/serialization/serialization_helper.tcc"
EXPLICIT_INSTANTIATE_BOOST_SERIALIZE(::marocco::placement::parameters::InputPlacement)
