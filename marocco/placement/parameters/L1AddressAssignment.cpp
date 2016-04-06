#include "marocco/placement/parameters/L1AddressAssignment.h"

#include <boost/serialization/nvp.hpp>

namespace marocco {
namespace placement {
namespace parameters {

L1AddressAssignment::L1AddressAssignment() : m_strategy(Strategy::high_first)
{
}

void L1AddressAssignment::strategy(Strategy value)
{
	m_strategy = value;
}

auto L1AddressAssignment::strategy() const -> Strategy
{
	return m_strategy;
}

template <typename Archive>
void L1AddressAssignment::serialize(Archive& ar, unsigned int const /* version */)
{
	using namespace boost::serialization;
	// clang-format off
	ar & make_nvp("strategy", m_strategy);
	// clang-format on
}

} // namespace parameters
} // namespace placement
} // namespace marocco

BOOST_CLASS_EXPORT_IMPLEMENT(::marocco::placement::parameters::L1AddressAssignment)

#include "boost/serialization/serialization_helper.tcc"
EXPLICIT_INSTANTIATE_BOOST_SERIALIZE(::marocco::placement::parameters::L1AddressAssignment)
