#pragma once

#ifndef PYPLUSPLUS
#include <unordered_map>
#endif // !PYPLUSPLUS

#include <boost/serialization/export.hpp>

#include "pywrap/compat/macros.hpp"

namespace boost {
namespace serialization {
class access;
} // namespace serialization
} // namespace boost

namespace marocco {
namespace routing {
namespace parameters {

class L1Routing {
public:
	typedef float priority_type;
	typedef size_t projection_type;

	L1Routing();

	PYPP_CLASS_ENUM(Algorithm)
	{
		backbone,
		dijkstra
	};

	PYPP_CLASS_ENUM(PriorityAccumulationMeasure)
	{
		// TODO: median, max, sum, geometric/harmonic mean…
		arithmetic_mean
	};

	/**
	 * @brief Order in which L1 switches are considered
	 *
	 * shuffle_switches_with_hicann_enum_as_seed: seed the random engine for each HICANN with its Enum
	 * shuffle_switches_with_given_seed: seed the random engine once with the seed given via shuffle_switches_seed(seed)
	 * switches_in_filled_in_order: use switches in the order they were filled in
	 *
	 * default: shuffle_switches_with_hicann_enum_as_seed
	 *
	 */
	PYPP_CLASS_ENUM(SwitchOrdering){
		shuffle_switches_with_hicann_enum_as_seed,
		shuffle_switches_with_given_seed,
		switches_in_filled_in_order
	};

	void algorithm(Algorithm value);
	Algorithm algorithm() const;

	/**
	 * @brief Sets the routing priority of the given projection.
	 * @param projection euter id of projection
	 * @param value priority ≥ 1.
	 * Projections with higher priority will tend to be routed earlier than projections
	 * with lower priority.  The extent to which this is the case depends on the actual
	 * placement of the corresponding populations, as projections starting from a common
	 * DNC merger will be routed at the same time.
	 * @throw std::invalid_argument If the specified priority is invalid.
	 */
	void priority(projection_type const& projection, priority_type value);

	/**
	 * @brief Returns the priority of the specified projection.
	 * @param projection euter id of projection
	 * @return Stored priority, defaulting to one.
	 */
	priority_type priority(projection_type const& projection) const;

	/**
	 * @brief Sets measure to use when consolidating priorities of projections starting
	 *        from a common DNC merger.
	 */
	void priority_accumulation_measure(PriorityAccumulationMeasure value);

	/**
	 * @brief Returns measure to use when consolidating priorities.
	 */
	PriorityAccumulationMeasure priority_accumulation_measure() const;

	/**
	 * @brief seed for random generator used for e.g. shuffling of switches
	 */
	void shuffle_switches_seed(size_t seed);
	size_t shuffle_switches_seed() const;

	void switch_ordering(SwitchOrdering value);
	SwitchOrdering switch_ordering() const;

private:
	Algorithm m_algorithm;
#ifndef PYPLUSPLUS
	std::unordered_map<projection_type, priority_type> m_priorities;
#endif // !PYPLUSPLUS
	PriorityAccumulationMeasure m_priority_accumulation_measure;
	SwitchOrdering m_switch_ordering;
	size_t m_shuffle_switches_seed;
	friend class boost::serialization::access;
	template <typename Archive>
	void serialize(Archive& ar, unsigned int const /* version */);
}; // L1Routing

} // namespace parameters
} // namespace routing
} // namespace marocco

BOOST_CLASS_EXPORT_KEY(::marocco::routing::parameters::L1Routing)
