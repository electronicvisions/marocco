#include "test/placement/test-MergerTreeRouter.h"

#include "hal/Coordinate/iter_all.h"

using namespace HMF::Coordinate;

namespace marocco {
namespace placement {

TEST_F(AMergerTreeRouter, collatesNeuronBlocks23)
{
	HICANNOnWafer hicann(Enum(0));
	NeuronBlockOnHICANN nb2(2);
	NeuronBlockOnHICANN nb3(3);
	NeuronBlockOnWafer nbw2(nb2, hicann);
	NeuronBlockOnWafer nbw3(nb3, hicann);

	add(nbw2, 12);
	add(nbw3, 32);

	auto router = build_router();
	router.run(hicann);

	DNCMergerOnHICANN dnc_merger(3);
	auto const& result = router.result();
	EXPECT_EQ(dnc_merger, result.at(nb2));
	EXPECT_EQ(dnc_merger, result.at(nb3));
}

TEST_F(AMergerTreeRouter, collatesNeuronBlocks34)
{
	// Neuron block 4 can be connected to DNC merger 3 because neuron block 5 is unused.
	HICANNOnWafer hicann(Enum(0));
	NeuronBlockOnHICANN nb3(3);
	NeuronBlockOnHICANN nb4(4);
	NeuronBlockOnWafer nbw3(nb3, hicann);
	NeuronBlockOnWafer nbw4(nb4, hicann);

	add(nbw3, 32);
	add(nbw4, 26);

	auto router = build_router();
	router.run(hicann);

	DNCMergerOnHICANN dnc_merger(3);
	auto const& result = router.result();
	EXPECT_EQ(dnc_merger, result.at(nb3));
	EXPECT_EQ(dnc_merger, result.at(nb4));
}

TEST_F(AMergerTreeRouter, doesNotCollateNeurons345)
{
	// Neuron block 4 can be connected to DNC merger 3 because that would use a switch needed for
	// neuron block 5.  The numbers were chosen s.t. we cannot connect all blocks to DNC merger 3.
	HICANNOnWafer hicann(Enum(0));
	NeuronBlockOnHICANN nb3(3);
	NeuronBlockOnHICANN nb4(4);
	NeuronBlockOnHICANN nb5(5);
	NeuronBlockOnWafer nbw3(nb3, hicann);
	NeuronBlockOnWafer nbw4(nb4, hicann);
	NeuronBlockOnWafer nbw5(nb5, hicann);

	add(nbw3, 32);
	add(nbw4, 26);
	add(nbw5, 32);

	Merger1OnHICANN m1{2};
	Merger3OnHICANN m3{0};

	auto router = build_router();
	router.run(hicann);

	auto const& result = router.result();
	ASSERT_EQ(DNCMergerOnHICANN{3}, result.at(NeuronBlockOnHICANN{3}));
	ASSERT_EQ(DNCMergerOnHICANN{5}, result.at(NeuronBlockOnHICANN{4}));
	ASSERT_EQ(DNCMergerOnHICANN{5}, result.at(NeuronBlockOnHICANN{5}));
}

TEST_F(AMergerTreeRouter, doesNotCollateBecauseOfLimitedCapacity)
{
	HICANNOnWafer hicann(Enum(0));
	for (auto nb : iter_all<NeuronBlockOnHICANN>()) {
		NeuronBlockOnWafer nbw(nb, hicann);
		add(nbw, 32);
	}

	auto router = build_router();
	router.run(hicann);

	auto const& result = router.result();
	ASSERT_EQ(8, result.size());
	for (auto nb : iter_all<NeuronBlockOnHICANN>()) {
		EXPECT_EQ(DNCMergerOnHICANN(nb), result.at(nb));
	}
}

TEST_F(AMergerTreeRouter, mergesAllNeuronBlocks)
{
	HICANNOnWafer hicann(Enum(0));
	for (auto nb : iter_all<NeuronBlockOnHICANN>()) {
		NeuronBlockOnWafer nbw(nb, hicann);
		add(nbw, 5);
	}

	auto router = build_router();
	router.run(hicann);

	auto const& result = router.result();
	ASSERT_EQ(8, result.size());
	for (auto nb : iter_all<NeuronBlockOnHICANN>()) {
		EXPECT_EQ(DNCMergerOnHICANN(3), result.at(nb));
	}
}

} // placement
} // marocco
