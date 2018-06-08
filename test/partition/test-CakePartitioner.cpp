#include "test/common.h"

#include <boost/make_shared.hpp>

#include "redman/backend/MockBackend.h"

#include "marocco/partition/CakePartitioner.h"

using namespace marocco::resource;
using namespace HMF::Coordinate;

namespace marocco {
namespace partition{


TEST(CakePartitioner, Basic)
{
	size_t const hicanns = 384;

	auto backend = boost::make_shared<redman::backend::MockBackend>();

	// FIXME: revisit this when ctor of HICANNManager has been adapted
	HICANNManager mgr(backend, {Wafer{}});

	size_t const iter = 100;
	for (size_t jj=0; jj<iter; ++jj)
	{
		size_t const N = 2 + rand() % 20;

		std::vector<size_t> num(N);
		for (size_t ii=0; ii<N; ++ii)
		{
			HICANNManager cpy(mgr);
			ASSERT_EQ(hicanns, cpy.count_present());

			// do the partitioning
			CakePartitioner part(cpy, N, ii);
			num.at(ii) = cpy.count_present();

			// make sure we have at least some hicanns in the partition
			ASSERT_LT(0, cpy.count_present()) << "N: " << N << " ii: " << ii;

			// make sure, we don't have all hicanns
			ASSERT_GT(hicanns, cpy.count_present())  << "N: " << N << " ii: " << ii;
		}

		// make sure every hicann is in exactly one partition
		size_t sum = 0;
		for (auto const& val : num)
			sum += val;

		ASSERT_EQ(hicanns, sum);
	}
}

} // partition
} // marocco
