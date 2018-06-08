#include "marocco/placement/SpiralHICANNOrdering.h"

#include <cmath>

namespace marocco {
namespace placement {

SpiralHICANNOrdering::SpiralHICANNOrdering(
	float const meanX,
	float const meanY) :
		mCenter{meanX, meanY}
{}

SpiralHICANNOrdering::SpiralHICANNOrdering(
	resource::HICANNManager const& mgr) :
		mCenter(init(mgr))
{}

bool SpiralHICANNOrdering::operator () (
	HICANNGlobal const& lhs,
	HICANNGlobal const& rhs) const
{
	if (lhs.toWafer() != rhs.toWafer())
		return lhs.toWafer() < rhs.toWafer();

	else if (radius(lhs) != radius(rhs))
		return radius(lhs) < radius(rhs);
	else
		return angle(lhs) < angle(rhs);
}

float SpiralHICANNOrdering::meanX() const
{
	return mCenter[0];
}

float SpiralHICANNOrdering::meanY() const
{
	return mCenter[1];
}

float SpiralHICANNOrdering::x(HICANNGlobal const& h) const
{
	return float(h.x()) - meanX();
}

float SpiralHICANNOrdering::y(HICANNGlobal const& h) const
{
	return float(h.y()) - meanY();
}

float SpiralHICANNOrdering::radius(HICANNGlobal const& h) const
{
	return std::max(abs(x(h)), abs(y(h)));
}

float SpiralHICANNOrdering::angle(HICANNGlobal const& h) const
{
	auto v = rotate(x(h), y(h), -M_PI/4.);
	return atan2(v[1] /*y*/, v[0] /*x*/);
}

std::valarray<float>
SpiralHICANNOrdering::rotate(
	float const x,
	float const y,
	float const angle) const
{
	return std::valarray<float> {
		x * cos(angle) - y * sin(angle),
		x * sin(angle) + y * cos(angle)
	};
}

std::valarray<float>
SpiralHICANNOrdering::init(resource::HICANNManager const& mgr)
{
	std::valarray<float> v {0, 0};
	size_t cnt = 0;
	for(auto const& h : mgr.present())
	{
		v += std::valarray<float> {h.x(), h.y()};
		cnt++;
	}
	v /= cnt;
	return v;
}

} // namespace placement
} // namespace marocco
