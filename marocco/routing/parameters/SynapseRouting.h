#pragma once

#include <boost/serialization/export.hpp>

namespace boost {
namespace serialization {
class access;
} // namespace serialization
} // namespace boost

namespace marocco {
namespace routing {
namespace parameters {

class SynapseRouting {
public:
	SynapseRouting();

	/**
	 * @brief Sets the maximum length of chained synapse drivers.
	 * As only a single synapse driver can be connected to a given vertical L1 bus, an
	 * analog synapse driver feature is used that mirrors L1 events to adjacent drivers
	 * s.t. enough drivers are available (e.g. to decode all source addresses present on
	 * the connected L1 bus).  If the length of those synapse driver chains exceeds a
	 * certain value, L1 events may fail to decode properly, due to the analog effects.
	 * @throw std::invalid_argument If the specified chain length does not fit on a quadrant.
	 */
	void driver_chain_length(size_t value);
	size_t driver_chain_length() const;

	/**
	 * @brief Configure synapse driver masks to discard non-background events.
	 */
	void only_allow_background_events(bool enable);
	bool only_allow_background_events() const;

private:
	size_t m_driver_chain_length;
	bool m_only_allow_background_events;

	friend class boost::serialization::access;
	template <typename Archive>
	void serialize(Archive& ar, unsigned int const /* version */);
}; // SynapseRouting

} // namespace parameters
} // namespace routing
} // namespace marocco

BOOST_CLASS_EXPORT_KEY(::marocco::routing::parameters::SynapseRouting)
