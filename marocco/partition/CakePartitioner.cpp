#include "marocco/partition/CakePartitioner.h"
#include "marocco/util.h"

#include <cmath>
#include <valarray>


namespace marocco {
namespace partition {


CakePartitioner::CakePartitioner(manager_type& mgr, size_t N, size_t rank) :
		mCenter(init(mgr))
{
	float const split_angle = M_PI*2 / N;
	for(auto const& h : mgr.present())
	{
		float const angle = M_PI + atan2(y(h), x(h));
		if (rank == size_t(angle/split_angle)) {
			// we have a winner
		} else {
			mgr.mask(h);
		}
	}
}


std::valarray<float> CakePartitioner::init(manager_type const& mgr)
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

float CakePartitioner::meanX() const
{
	return mCenter[0];
}

float CakePartitioner::meanY() const
{
	return mCenter[1];
}

float CakePartitioner::x(HMF::Coordinate::HICANNGlobal const& h) const
{
	return float(h.x()) - meanX();
}

float CakePartitioner::y(HMF::Coordinate::HICANNGlobal const& h) const
{
	return float(h.y()) - meanY();
}

} // namespace partition
} // namespace marocco
