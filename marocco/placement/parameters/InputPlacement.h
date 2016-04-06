#pragma once

#include <boost/serialization/export.hpp>

namespace boost {
namespace serialization {
class access;
} // namespace serialization
} // namespace boost

namespace marocco {
namespace placement {
namespace parameters {

class InputPlacement {
public:
	InputPlacement();

	/**
	 * @brief Consider firing rate of spike sources and available input bandwidth.
	 * To avoid spike loss during the experiment, spike sources will be placed in such a
	 * way that per-FPGA and per-HICANN limits for the input bandwidth are not exceeded.
	 */
	void consider_firing_rate(bool enable);
	bool consider_firing_rate() const;

	/**
	 * @brief Utilization of available bandwidth to assume.
	 * @param fraction Floating point number in [0.0, 1.0]
	 * @throw std::invalid_argument If \c fraction is invalid.
	 * @see #consider_firing_rate().
	 * As only the mean firing rate of spike sources is known, one has to account for the
	 * possibility of temporarily higher rates.  By using a smaller fraction of the full
	 * bandwidth per HICANN / FPGA, there will be more headroom.
	 */
	void bandwidth_utilization(double fraction);
	double bandwidth_utilization() const;

private:
	bool m_consider_firing_rate;
	double m_bandwidth_utilization;

	friend class boost::serialization::access;
	template <typename Archive>
	void serialize(Archive& ar, unsigned int const /* version */);
}; // InputPlacement

} // namespace parameters
} // namespace placement
} // namespace marocco

BOOST_CLASS_EXPORT_KEY(::marocco::placement::parameters::InputPlacement)
