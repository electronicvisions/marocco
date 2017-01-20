#include "marocco/experiment/parameters/Experiment.h"

#include <stdexcept>
#include <boost/serialization/nvp.hpp>

namespace marocco {
namespace experiment {
namespace parameters {

Experiment::Experiment()
	: m_bio_duration_in_s(0.),
	  m_speedup(1e4),
	  m_offset_in_s(20e-6),
	  m_discard_background_events(true)
{
}

void Experiment::bio_duration_in_s(double seconds)
{
	if (seconds < 0.) {
		throw std::invalid_argument("experiment duration cannot be negative");
	}
	m_bio_duration_in_s = seconds;
}

double Experiment::bio_duration_in_s() const
{
	return m_bio_duration_in_s;
}

double Experiment::hardware_duration_in_s() const
{
	return m_offset_in_s + m_bio_duration_in_s / m_speedup;
}

void Experiment::speedup(double factor)
{
	if (factor < 0.) {
		throw std::invalid_argument("speedup factor cannot be negative");
	}
	m_speedup = factor;
}

double Experiment::speedup() const
{
	return m_speedup;
}

void Experiment::offset_in_s(double seconds)
{
	if (seconds < 0.) {
		throw std::invalid_argument("time offset cannot be negative");
	}
	m_offset_in_s = seconds;
}

double Experiment::offset_in_s() const
{
	return m_offset_in_s;
}

void Experiment::discard_background_events(bool enable)
{
	m_discard_background_events = enable;
}

bool Experiment::discard_background_events() const
{
	return m_discard_background_events;
}

template <typename Archive>
void Experiment::serialize(Archive& ar, unsigned int const /* version */)
{
	using namespace boost::serialization;
	// clang-format off
	ar & make_nvp("bio_duration_in_s", m_bio_duration_in_s)
	   & make_nvp("speedup", m_speedup)
	   & make_nvp("offset_in_s", m_offset_in_s)
	   & make_nvp("discard_background_events", m_discard_background_events);
	// clang-format on
}

} // namespace parameters
} // namespace experiment
} // namespace marocco

BOOST_CLASS_EXPORT_IMPLEMENT(::marocco::experiment::parameters::Experiment)

#include "boost/serialization/serialization_helper.tcc"
EXPLICIT_INSTANTIATE_BOOST_SERIALIZE(::marocco::experiment::parameters::Experiment)
