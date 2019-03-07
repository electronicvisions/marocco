#include "test/placement/test-MergerTreeRouter.h"

#include "hal/Coordinate/iter_all.h"
#include "marocco/placement/MergerTreeConfigurator.h"

using namespace HMF::Coordinate;
using namespace HMF::HICANN;

namespace marocco {
namespace placement {

class AMergerTreeConfigurator : public AMergerTreeRouter
{
public:
	void run_configurator() {
		auto router = build_router();
		router.run(HICANNOnWafer(Enum(0)));
		mapping = router.result();
		MergerTreeConfigurator configurator(layer1, graph);
		configurator.run(mapping);
	}

	sthal::Layer1 layer1;
	MergerTreeRouter::result_type mapping;
}; // AMergerTreeConfigurator

TEST_F(AMergerTreeConfigurator, collatesNeuronBlocks23)
{
	HICANNOnWafer hicann(Enum(0));
	NeuronBlockOnHICANN nb2(2);
	NeuronBlockOnHICANN nb3(3);
	NeuronBlockOnWafer nbw2(nb2, hicann);
	NeuronBlockOnWafer nbw3(nb3, hicann);

	add(nbw2, 12);
	add(nbw3, 32);

	Merger1OnHICANN merger(1);

	EXPECT_EQ(Merger::RIGHT_ONLY, layer1[merger].config.to_ulong());

	run_configurator();

	EXPECT_EQ(Merger::MERGE, layer1[merger].config.to_ulong());
}

TEST_F(AMergerTreeConfigurator, collatesNeuronBlocks34)
{
	// Neuron block 4 can be connected to DNC merger 3 because neuron block 5 is unused.
	HICANNOnWafer hicann(Enum(0));
	NeuronBlockOnHICANN nb3(3);
	NeuronBlockOnHICANN nb4(4);
	NeuronBlockOnWafer nbw3(nb3, hicann);
	NeuronBlockOnWafer nbw4(nb4, hicann);

	add(nbw3, 32);
	add(nbw4, 26);

	Merger1OnHICANN m1{2};
	Merger3OnHICANN m3{0};

	EXPECT_EQ(Merger::RIGHT_ONLY, layer1[m1].config.to_ulong());
	EXPECT_EQ(Merger::LEFT_ONLY, layer1[m3].config.to_ulong());

	run_configurator();

	auto flag = layer1[m1].config.to_ulong();
	// Merger1OnHICANN(2) should include its left input (from neuron block 4)
	EXPECT_TRUE(flag == Merger::LEFT_ONLY || flag == Merger::MERGE) << flag;
	EXPECT_EQ(Merger::MERGE, layer1[m3].config.to_ulong());
}

TEST_F(AMergerTreeConfigurator, doesNotCollateNeurons345)
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

	EXPECT_EQ(Merger::RIGHT_ONLY, layer1[m1].config.to_ulong());
	EXPECT_EQ(Merger::LEFT_ONLY, layer1[m3].config.to_ulong());

	run_configurator();

	EXPECT_EQ(Merger::MERGE, layer1[m1].config.to_ulong());
	// Merger3OnHICANN(0) only uses its left input (from neuron block 3)
	EXPECT_EQ(Merger::LEFT_ONLY, layer1[m3].config.to_ulong());
}

TEST_F(AMergerTreeConfigurator, doesNotCollateBecauseOfLimitedCapacity)
{
	HICANNOnWafer hicann(Enum(0));
	for (auto nb : iter_all<NeuronBlockOnHICANN>()) {
		NeuronBlockOnWafer nbw(nb, hicann);
		add(nbw, 32);
	}

	run_configurator();

	EXPECT_EQ(sthal::Layer1(), layer1);
}

TEST_F(AMergerTreeConfigurator, producesRightConfigForMaxSPL1Case)
{
	MergerTreeRouter::result_type mapping;
	mapping[NeuronBlockOnHICANN(0)] = DNCMergerOnHICANN(0);
	mapping[NeuronBlockOnHICANN(1)] = DNCMergerOnHICANN(1);
	mapping[NeuronBlockOnHICANN(2)] = DNCMergerOnHICANN(2);
	mapping[NeuronBlockOnHICANN(3)] = DNCMergerOnHICANN(3);
	mapping[NeuronBlockOnHICANN(4)] = DNCMergerOnHICANN(4);
	mapping[NeuronBlockOnHICANN(5)] = DNCMergerOnHICANN(5);
	mapping[NeuronBlockOnHICANN(6)] = DNCMergerOnHICANN(6);

	MergerTreeConfigurator configurator(layer1, graph);
	configurator.run(mapping);

	EXPECT_EQ(sthal::Layer1(), layer1);
}

TEST_F(AMergerTreeConfigurator, disallowsCrossingAssignments)
{
	// BV: “Bad mappings like: NB0->DNCMerger(3) and NB1->DNCMerger(1). This input would
	// lead to a configuration, where spikes from NB0 and NB1 are forwarded and processed
	// by DNCMerger 1 and 3! Hence, a duplication of events!”

	MergerTreeRouter::result_type mapping;
	mapping[NeuronBlockOnHICANN(0)] = DNCMergerOnHICANN(3);
	mapping[NeuronBlockOnHICANN(1)] = DNCMergerOnHICANN(1);

	ASSERT_ANY_THROW({
		MergerTreeConfigurator configurator(layer1, graph);
		configurator.run(mapping);
	});
}

TEST_F(AMergerTreeConfigurator, disallowsImpossibleMappings)
{
	MergerTreeRouter::result_type mapping;
	mapping[NeuronBlockOnHICANN(0)] = DNCMergerOnHICANN(7);

	ASSERT_ANY_THROW({
		MergerTreeConfigurator configurator(layer1, graph);
		configurator.run(mapping);
	});
}

} // placement
} // marocco
