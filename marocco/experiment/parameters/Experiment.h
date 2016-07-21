#pragma once

#include <boost/serialization/export.hpp>

#include "pywrap/compat/macros.hpp"

namespace boost {
namespace serialization {
class access;
} // namespace serialization
} // namespace boost

namespace marocco {
namespace experiment {
namespace parameters {

class Experiment {
public:
	Experiment();

	/**
	 * @brief Duration of experiment.
	 */
	void bio_duration_in_s(double seconds);
	double bio_duration_in_s() const;

	double hardware_duration_in_s() const;

	/**
	 * @brief Speedup of emulation on hardware compared to biological experiment definition.
	 */
	void speedup(double factor);
	double speedup() const;

	/**
	 * @brief Initial delay of experiment start on hardware.
	 * This offset is required to ensure locking of the Layer 1 repeater PLLs.
	 * Furthermore, a minimal delay of around 500 nS is required so that external pulses
	 * can be injected into L1 at time 0 of the real experiment.
	 * @note When using the ESS backend, a value of 5e-7 is sufficient.
	 */
	void offset_in_s(double seconds);
	double offset_in_s() const;

private:
	double m_bio_duration_in_s;
	double m_speedup;
	double m_offset_in_s;

	friend class boost::serialization::access;
	template <typename Archive>
	void serialize(Archive& ar, unsigned int const /* version */);
}; // Experiment

} // namespace parameters
} // namespace experiment
} // namespace marocco

BOOST_CLASS_EXPORT_KEY(::marocco::experiment::parameters::Experiment)
