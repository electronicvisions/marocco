#include "marocco/routing/HardwareProjection.h"
#include "marocco/Logger.h"

namespace marocco {
namespace routing {

HardwareProjection::source_type&
HardwareProjection::source()
{
	return mSource;
}

HardwareProjection::source_type const&
HardwareProjection::source() const
{
	return mSource;
}

HardwareProjection::projection_type&
HardwareProjection::projection()
{
	return mProjection;
}

HardwareProjection::projection_type const&
HardwareProjection::projection() const
{
	return mProjection;
}

size_t HardwareProjection::size() const
{
	return mSource.size();
}

HardwareProjection::HardwareProjection(
	source_type const& source,
	projection_type const& proj) :
	mSource(source),
	mProjection(proj)
{}

HardwareProjection::HardwareProjection() :
	mSource({{},{}}),
	mProjection() {}

bool HardwareProjection::operator== (HardwareProjection const& rhs) const
{
	return (mProjection == rhs.mProjection) && (mSource == rhs.mSource);
}

} // namespace routing
} // namespace marocco
