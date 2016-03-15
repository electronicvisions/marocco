#include <unordered_set>
#include "test/common.h"

#include <boost/make_shared.hpp>

#include "redman/backend/MockBackend.h"

#include "marocco/placement/MergerTree.h"

using namespace HMF::Coordinate;

namespace marocco {
namespace placement {


class MergerTreeTest : public ::testing::Test
{
public:
	virtual void SetupUp() {}
	virtual void TearDown() {}

	MergerTreeTest() :
		hicann_coord(),
		hicann(),
		nb_mapping(),
		mgr(boost::make_shared<redman::backend::MockBackend>(),
		    {Wafer{}})
	{}

	void simple_fill(NeuronBlockOnHICANN nb, size_t count)
	{
		NeuronPlacementRequest dummy{assignment::PopulationSlice{{}, 0 /*offset*/, 1 /*size*/},
		                             2 /*hw_neuron_size*/};

		auto& onb = nb_mapping.at(nb);

		ASSERT_EQ(2, dummy.size());

		for (size_t ii = 0; ii < count; ++ii)
		{
			ASSERT_NE(onb.end(), onb.add(dummy)) << ii;
		}
	}

	MergerTreeRouter build()
	{
		return {hicann_coord, nb_mapping, hicann, mgr};
	}

	HICANNGlobal const hicann_coord;
	sthal::HICANN hicann;
	NeuronBlockMapping nb_mapping;
	resource_manager_t mgr;
};

TEST_F(MergerTreeTest, Dimension)
{
	auto merger_routing = build();

	ASSERT_EQ(14+8, num_edges(merger_routing.graph()));

	merger_routing.run();
}

TEST_F(MergerTreeTest, CollatesNeuronBlocks)
{
	using namespace HMF::HICANN;

	simple_fill(NeuronBlockOnHICANN{2}, 12);
	simple_fill(NeuronBlockOnHICANN{3}, 32);
	auto merger_routing = build();

	Merger1OnHICANN m{1};

	// nice bitset in your public API there… :P
	ASSERT_EQ(Merger::RIGHT_ONLY, hicann.layer1[m].config.to_ulong());

	merger_routing.run();

	ASSERT_EQ(Merger::MERGE, hicann.layer1[m].config.to_ulong());
}

// SJ@JK still relevant?
TEST_F(MergerTreeTest, DISABLED_LineSkipEdgeCase1)
{
	using namespace HMF::HICANN;

	// CASE 1: 4 can be connected to line 3 because line 5 is unused
	simple_fill(NeuronBlockOnHICANN{3}, 32);
	simple_fill(NeuronBlockOnHICANN{4}, 30);
	auto merger_routing = build();

	Merger1OnHICANN m1{2};
	Merger3OnHICANN m3{0};

	ASSERT_EQ(Merger::LEFT_ONLY, hicann.layer1[m3].config.to_ulong()) << "pre: 3/0 left";
	ASSERT_EQ(Merger::RIGHT_ONLY, hicann.layer1[m1].config.to_ulong()) << "pre: 1/2 right";

	merger_routing.run();

	auto const& result = merger_routing.result();
	ASSERT_EQ(2, result.size());
	ASSERT_EQ(DNCMergerOnHICANN{3}, result.at(NeuronBlockOnHICANN{3}));
	ASSERT_EQ(DNCMergerOnHICANN{3}, result.at(NeuronBlockOnHICANN{4}));

	auto flag = hicann.layer1[m1].config.to_ulong();
	ASSERT_TRUE(flag == Merger::LEFT_ONLY || flag == Merger::MERGE) << "post: 1/2 uses line 4";
	ASSERT_EQ(Merger::MERGE, hicann.layer1[m3].config.to_ulong()) << "post: 3/0 merges";
}

// SJ@JK still relevant?
TEST_F(MergerTreeTest, DISABLED_LineSkipEdgeCase2)
{
	using namespace HMF::HICANN;

	// CASE 2: 4 can't be connected to line 3 because that would use a switch on line 5.
	simple_fill(NeuronBlockOnHICANN{3}, 32);
	simple_fill(NeuronBlockOnHICANN{4}, 30);
	simple_fill(NeuronBlockOnHICANN{5}, 32);
	auto merger_routing = build();

	Merger1OnHICANN m1{2};
	Merger3OnHICANN m3{0};

	ASSERT_EQ(Merger::LEFT_ONLY, hicann.layer1[m3].config.to_ulong()) << "pre: 3/0 left";
	ASSERT_EQ(Merger::RIGHT_ONLY, hicann.layer1[m1].config.to_ulong()) << "pre: 1/2 right";

	merger_routing.run();

	auto const& result = merger_routing.result();
	ASSERT_EQ(3, result.size());
	ASSERT_EQ(DNCMergerOnHICANN{3}, result.at(NeuronBlockOnHICANN{3}));
	ASSERT_EQ(DNCMergerOnHICANN{5}, result.at(NeuronBlockOnHICANN{4}));
	ASSERT_EQ(DNCMergerOnHICANN{5}, result.at(NeuronBlockOnHICANN{5}));

	ASSERT_EQ(Merger::LEFT_ONLY, hicann.layer1[m3].config.to_ulong()) << "post: 3/0 left";
	ASSERT_EQ(Merger::MERGE, hicann.layer1[m1].config.to_ulong()) << "post: 1/2 right";
}

TEST_F(MergerTreeTest, DoesNotOverdoIt)
{
	for (size_t ii = 0; ii < 8; ++ii) {
		simple_fill(NeuronBlockOnHICANN{ii}, 32);
	}

	auto old = hicann.layer1;
	auto merger_routing = build();

	merger_routing.run();

	auto const& result = merger_routing.result();
	ASSERT_EQ(8, result.size())
	    << "All neuron blocks should have an output buffer.";

	std::unordered_set<DNCMergerOnHICANN> mergers;
	for (auto const& pair : result) {
		ASSERT_TRUE(mergers.insert(pair.second).second)
		    << "Each block should have a different output buffer";
	}

	// Switch settings should be equal to default, except:
	// For the DNC Mergers, in order to have slow=1, one has to set mode to MERGE(cf. #1369)
	// This is true for the default setting, but now for the MergerTreeRouter, which uses RIGHT_ONLY.
	// Instead, this is considered in InputPlacement::configureGbitLinks().
	// hence: check everything except of dnc mergers
	ASSERT_EQ(old.mMergerTree, hicann.layer1.mMergerTree) << "Merger routing changed Layer1 config.";
	ASSERT_EQ(old.mBackgroundGenerators, hicann.layer1.mBackgroundGenerators) << "Merger routing changed Layer1 config.";
	ASSERT_EQ(old.mGbitLink, hicann.layer1.mGbitLink) << "Merger routing changed Layer1 config.";
}

TEST_F(MergerTreeTest, MergesAllNeuronBlocks)
{
	for (size_t ii = 0; ii < 8; ++ii) {
		simple_fill(NeuronBlockOnHICANN{ii}, 5);
	}

	auto merger_routing = build();

	merger_routing.run();

	auto const& result = merger_routing.result();
	ASSERT_EQ(8, result.size())
	    << "All neuron blocks should have an output buffer.";

	auto const& ob = result.begin()->second;
	for (auto const& pair : result) {
		ASSERT_EQ(ob, pair.second)
		    << "All blocks should have the same output buffer";
	}
}

} // placement
} // marocco
