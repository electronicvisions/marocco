#include "marocco/routing/parameters/L1Routing.h"

#include <stdexcept>
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/unordered_map.h>

namespace marocco {
namespace routing {
namespace parameters {

L1Routing::L1Routing()
	: m_algorithm(Algorithm::backbone),
	  m_priority_accumulation_measure(PriorityAccumulationMeasure::arithmetic_mean),
	  m_switch_ordering(SwitchOrdering::shuffle_switches_with_hicann_enum_as_seed),
	  m_shuffle_switches_seed(424242)
{
}

void L1Routing::algorithm(Algorithm value)
{
	m_algorithm = value;
}

auto L1Routing::algorithm() const -> Algorithm
{
	return m_algorithm;
}

void L1Routing::switch_ordering(SwitchOrdering value)
{
	m_switch_ordering = value;
}

auto L1Routing::switch_ordering() const -> SwitchOrdering
{
	return m_switch_ordering;
}

void L1Routing::priority(projection_type const& projection, priority_type value)
{
	if (value < priority_type{1}) {
		throw std::invalid_argument("priority has to be larger than one");
	}
	m_priorities[projection] = value;
}

auto L1Routing::priority(projection_type const& projection) const -> priority_type
{
	auto it = m_priorities.find(projection);
	if (it == m_priorities.end()) {
		return priority_type{1};
	}
	return it->second;
}

void L1Routing::priority_accumulation_measure(PriorityAccumulationMeasure value)
{
	m_priority_accumulation_measure = value;
}

auto L1Routing::priority_accumulation_measure() const -> PriorityAccumulationMeasure
{
	return m_priority_accumulation_measure;
}

void L1Routing::shuffle_switches_seed(size_t shuffle_switches_seed)
{
	m_shuffle_switches_seed = shuffle_switches_seed;
}

auto L1Routing::shuffle_switches_seed() const -> size_t
{
	return m_shuffle_switches_seed;
}

template <typename Archive>
void L1Routing::serialize(Archive& ar, unsigned int const /* version */)
{
	using namespace boost::serialization;
	// clang-format off
	ar & make_nvp("algorithm", m_algorithm)
	   & make_nvp("priorities", m_priorities)
	   & make_nvp("priority_accumulation_measure", m_priority_accumulation_measure)
	   & make_nvp("switch_ordering", m_switch_ordering)
	   & make_nvp("shuffle_switches_seed", m_shuffle_switches_seed);
	// clang-format on
}

} // namespace parameters
} // namespace routing
} // namespace marocco

BOOST_CLASS_EXPORT_IMPLEMENT(::marocco::routing::parameters::L1Routing)

#include "boost/serialization/serialization_helper.tcc"
EXPLICIT_INSTANTIATE_BOOST_SERIALIZE(::marocco::routing::parameters::L1Routing)
