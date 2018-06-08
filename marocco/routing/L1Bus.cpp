#include "marocco/routing/L1Bus.h"

using namespace HMF::Coordinate;

namespace marocco {
namespace routing {

L1Bus::L1Bus(direction_t dir, busid_t id, HICANNGlobal hicann) :
	mDirection(dir),
	mBusId(id),
	mHICANN(hicann)
{}

L1Bus::L1Bus() :
	mDirection(),
	mBusId(),
	mHICANN()
{}

L1Bus::direction_t
L1Bus::getDirection() const
{
	return mDirection;
}

L1Bus::busid_t
L1Bus::getBusId() const
{
	return mBusId;
}

VLineOnHICANN L1Bus::toVLine() const
{
	if (mDirection!=Vertical) {
		throw WrongBusType();
	}
	return VLineOnHICANN(mBusId);
}

HLineOnHICANN L1Bus::toHLine() const
{
	if (mDirection!=Horizontal) {
		throw WrongBusType();
	}
	return HLineOnHICANN(mBusId);
}

HICANNGlobal
L1Bus::hicann() const
{
	return mHICANN;
}

Side L1Bus::side() const
{
	// inserted left or right?
	return (getBusId() < 128) ? geometry::left : geometry::right;
}

bool L1Bus::operator== (L1Bus const& rhs) const
{
	return (mHICANN == rhs.mHICANN) &&
		(mBusId == rhs.mBusId) &&
		(mDirection == rhs.mDirection);
}

bool L1Bus::operator!= (L1Bus const& rhs) const
{
	return !(*this == rhs);
}

} // namespace routing
} // namespace marocco
