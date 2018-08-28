#include "test/common.h"

#include <vector>

#include "marocco/placement/algorithms/ClusterByPopulationConnectivity.h"
#include "marocco/placement/algorithms/PlacePopulationsBase.h"
#include "marocco/placement/algorithms/bySmallerNeuronBlockAndPopulationID.h"
//#include "marocco/util.h"

namespace marocco {
namespace placement {
namespace algorithms {

class PlacementTest : public ::testing::Test
{};

TEST_F(PlacementTest, comparable)
{
	bySmallerNeuronBlockAndPopulationID placer_linear;
	PlacePopulationsBase placer_base;
	PlacePopulationsBase placer;

	ASSERT_TRUE(placer == placer_base);
	ASSERT_FALSE(placer == placer_linear);
}

TEST_F(PlacementTest, clusterPop)
{
	ClusterByPopulationConnectivity placer1;
	ClusterByPopulationConnectivity placer2;

	ASSERT_TRUE(placer1 == placer2);
}

} // namespace internal
} // namespace placement
} // namespace marocco
