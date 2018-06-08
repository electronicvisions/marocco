#include "test/test-DistributedMap.h"
#include <type_traits>

namespace marocco {

typedef DistributedMap<graph_t, int> map_t;
typedef map_t::value_type value;

static_assert(std::is_same<map_t::key_type, key>::value,
			  "key_type should be same type as graph_t::vertex_descriptor");

TEST(DistributedMap, Basic)
{
	map_t map;

	int const rank = MPI::COMM_WORLD.Get_rank();
	int const size = MPI::COMM_WORLD.Get_size();

	size_t const N = 100;

	REPEAT(N,
		key const k = make_key(__ii, rank);

		int const r = rand();
		map.local_put(k, r);
		ASSERT_EQ(r, map.get(k));

		int const q = rand();
		map.put(k, q);
		ASSERT_EQ(q, map.get(k));
	);
	ASSERT_EQ(N, map.size());

	// clear internal representation
	map.mMap.clear();
	ASSERT_EQ(0, map.size());

	REPEAT(N,
		key const k = make_key(__ii, rank);
		ASSERT_THROW(map.get(k), std::out_of_range);
	);
}

TEST(DistributedMap, NonExistant)
{
	map_t map;
	ASSERT_THROW(map.get(make_key(5)), std::out_of_range);
}

} // marocco
