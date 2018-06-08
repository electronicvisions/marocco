#pragma once
#include <valarray>

#include "hal/Coordinate/HMFGeometry.h"
#include "marocco/resource/HICANNManager.h"

namespace marocco {
namespace placement {

// Sort HICANNGlobal Coordinates spirally outwards
class SpiralHICANNOrdering
{
public:
	SpiralHICANNOrdering(float meanX = 17.5, float meanY = 7.5);
	SpiralHICANNOrdering(resource::HICANNManager const& mgr);

	typedef HMF::Coordinate::HICANNGlobal HICANNGlobal;

	bool operator () (HICANNGlobal const& lhs, HICANNGlobal const& rhs) const;

	float meanX() const;
	float meanY() const;

	float x(HICANNGlobal const& h) const;
	float y(HICANNGlobal const& h) const;

	float radius(HICANNGlobal const& h) const;
	float angle(HICANNGlobal const& h) const;
	std::valarray<float> rotate(float x, float y, float angle) const;

private:
	static std::valarray<float> init(resource::HICANNManager const& mgr);

	std::valarray<float> mCenter;

	friend class SpiralHICANNOrderingTest;
};

} // namespace placement
} // namespace marocco
