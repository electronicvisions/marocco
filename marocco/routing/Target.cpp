#include "marocco/routing/Target.h"

using namespace halco::hicann::v2;
using namespace halco::common;

namespace marocco {
namespace routing {

Target::Target(HICANNOnWafer const& hicann, Orientation const& orientation)
    : m_hicann(hicann), m_orientation(orientation)
{
}

HICANNOnWafer Target::toHICANNOnWafer() const
{
	return m_hicann;
}

Orientation Target::toOrientation() const
{
	return m_orientation;
}

bool Target::operator==(Target const& other) const
{
	return m_hicann == other.m_hicann && m_orientation == other.m_orientation;
}

} // namespace routing
} // namespace marocco
