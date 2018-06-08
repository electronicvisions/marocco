#include <list>
#include "test/common.h"

#include <boost/make_shared.hpp>

#include "redman/backend/MockBackend.h"

#include "marocco/placement/SpiralHICANNOrdering.h"

using namespace std;
using namespace HMF::Coordinate;

namespace marocco {
namespace placement {

class SpiralHICANNOrderingTest :
	public ::testing::Test
{};

TEST_F(SpiralHICANNOrderingTest, DefaultArguemnts)
{
	SpiralHICANNOrdering sho;
	ASSERT_EQ(17.5, sho.meanX());
	ASSERT_EQ( 7.5, sho.meanY());
}

TEST_F(SpiralHICANNOrderingTest, Constructor)
{
	auto backend = boost::make_shared<redman::backend::MockBackend>();

	// FIXME: revisit this when ctor of HICANNManager has been adapted
	resource::HICANNManager mgr(backend, {Wafer{}});
	SpiralHICANNOrdering sho(mgr);

	ASSERT_EQ(17.5, sho.meanX());
	ASSERT_EQ( 7.5, sho.meanY());
}

TEST_F(SpiralHICANNOrderingTest, Ordering)
{
	set<HICANNGlobal, SpiralHICANNOrdering> s;

	for (size_t ii=0; ii <384; ++ii)
		s.insert(HICANNGlobal(Enum(ii)));
	ASSERT_EQ(384, s.size());

	std::array<std::array<int,
		HICANNGlobal::y_type::end>,
		HICANNGlobal::x_type::end> res;

	// reset all the entries
	for (auto& o : res)
		for (auto& i : o)
			i = -1;

	// write hicann ordering to grid
	std::list<HICANNGlobal> ll;
	{
		int cnt = 0;
		for (auto const& h : s)
		{
			res.at(h.x()).at(h.y()) = cnt++;
			ll.push_back(h);
		}
	}

	// check topological adjacency
	for (size_t yy = 0; yy<HICANNGlobal::y_type::end; ++yy)
	{
		for (size_t xx = 0; xx<HICANNGlobal::x_type::end; ++xx)
		{
			int const val = res.at(xx).at(yy);

			// ignore uninitialized entries
			if (val < 0)
				continue;

			// check distance from center is monotoniously increasing
			{
				SpiralHICANNOrdering sho;
				static float last_distance = 0.;
				float const distance = sho.radius(ll.front());

				ASSERT_LT(0, ll.size());
				ll.pop_front();
				ASSERT_LE(last_distance, distance);
				last_distance = distance;
			}

			// avoid corner cases
			if (yy == 0 || yy == HICANNGlobal::y_type::max || \
				xx == 0 || xx == HICANNGlobal::x_type::max)
				continue;

			std::vector<int> adj {
				res.at(xx).at(yy-1),
				res.at(xx).at(yy+1),
				res.at(xx-1).at(yy),
				res.at(xx+1).at(yy)
			};

			// avoid more corner cases
			if (adj.end() != std::find(adj.begin(), adj.end(), -1))
				continue;

			// check that any of the adjacent elements is sucessor
			if (val<383)
				ASSERT_NE(adj.end(), std::find(adj.begin(), adj.end(), val+1)) << val;

			// check that any of the adjacent elements is predecessor
			if (val>0)
				ASSERT_NE(adj.end(), std::find(adj.begin(), adj.end(), val-1)) << val;
		}
	}
}

} // placement
} // marocco
