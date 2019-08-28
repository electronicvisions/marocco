#include "marocco/placement/parameters/MergerRouting.h"

#include <boost/serialization/nvp.hpp>

namespace marocco {
namespace placement {
namespace parameters {

MergerRouting::MergerRouting() : m_strategy(Strategy::minimize_as_possible) {}

void MergerRouting::strategy(Strategy value)
{
	m_strategy = value;
}

auto MergerRouting::strategy() const -> Strategy
{
	return m_strategy;
}

template <typename Archive>
void MergerRouting::serialize(Archive& ar, unsigned int const /* version */)
{
	using namespace boost::serialization;
	// clang-format off
	ar & make_nvp("strategy", m_strategy);
	// clang-format on
}

} // namespace parameters
} // namespace placement
} // namespace marocco

BOOST_CLASS_EXPORT_IMPLEMENT(::marocco::placement::parameters::MergerRouting)

#include "boost/serialization/serialization_helper.tcc"
EXPLICIT_INSTANTIATE_BOOST_SERIALIZE(::marocco::placement::parameters::MergerRouting)
