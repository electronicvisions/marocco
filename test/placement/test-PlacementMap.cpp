#include "test/common.h"
#include "test/test-DistributedMap.h"
#include "marocco/placement/PlacementMap.h"

namespace marocco {
namespace placement {

TEST(PlacementMap, Basic)
{
	typedef PlacementMap::key_type key;
	typedef PlacementMap::value_type value;


	boost::mpi::communicator comm(MPI::COMM_WORLD, boost::mpi::comm_attach);
	PlacementMap pm;

	int const rank = MPI::COMM_WORLD.Get_rank();
	int const size = MPI::COMM_WORLD.Get_size();
	size_t val = 0;

	pm.put(make_key(val++, rank), value());
	ASSERT_EQ(1, pm.size()) << rank;

	pm.local_put(make_key(val++, rank), value());
	ASSERT_EQ(2, pm.size()) << rank;

	// try to locally put non-local key -> shouldn't throw but also should not
	// increase local map size.
	//ASSERT_NO_THROW(pm.local_put(make_key(val++, rank+1), value())) << rank+1;
	//ASSERT_EQ(2, pm.size());

	// try to locally put non local key
	ASSERT_NO_THROW(pm.local_put(make_key(val++, (rank+1)%size), value())) << (rank+1)%size;

	if (!threaded_mpi())
		return;

	// synchronize - and afterwards all process should have an extra key
	// received from `rank-1`.
	pm.synchronize();
	ASSERT_EQ(3, pm.size());

	//make sure default constructed mpi_process_group uses MPI world
	//communicator.
	//ASSERT_EQ(rank, pm.process().rank);
	//ASSERT_EQ(size, pm.process().size);
}

} // placement
} // marocco
