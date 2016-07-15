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

private:
	Algorithm m_algorithm;
#ifndef PYPLUSPLUS
	std::unordered_map<projection_type, priority_type> m_priorities;
#endif // !PYPLUSPLUS
	PriorityAccumulationMeasure m_priority_accumulation_measure;

	friend class boost::serialization::access;
	template <typename Archive>
	void serialize(Archive& ar, unsigned int const /* version */);
}; // L1Routing

} // namespace parameters
} // namespace routing
} // namespace marocco

BOOST_CLASS_EXPORT_KEY(::marocco::routing::parameters::L1Routing)
