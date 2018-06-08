#pragma once

#include <valarray>

#include "hal/Coordinate/HMFGeometry.h"
#include "marocco/resource/HICANNManager.h"

// TODO: lots of code redundant with SpiralHICANNOrdering

namespace marocco {
namespace partition {

class CakePartitioner
{
public:
	typedef resource::HICANNManager manager_type;

	CakePartitioner(manager_type& mgr, size_t N, size_t rank);

private:
	// TODO: lots of code redundant with SpiralHICANNOrdering
	static std::valarray<float> init(manager_type const& mgr);

	float meanX() const;
	float meanY() const;

	float x(HMF::Coordinate::HICANNGlobal const& h) const;
	float y(HMF::Coordinate::HICANNGlobal const& h) const;

private:
	std::valarray<float> mCenter;
};

} // namespace partition
} // namespace marocco
