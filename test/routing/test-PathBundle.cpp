#include "test/common.h"

#include "marocco/routing/PathBundle.h"

using namespace HMF::Coordinate;

namespace marocco {
namespace routing {

class APathBundle : public ::testing::Test
{
public:
	APathBundle()
	{
		rgraph.add(hicann);
		PathBundle::path_type path;
		path.push_back(rgraph[hicann][source]);
		path.push_back(rgraph[hicann][VLineOnHICANN(39)]);
		bundle.add(path);
	}

protected:
	HICANNOnWafer hicann = HICANNOnWafer(Enum(0));
	HLineOnHICANN source = HLineOnHICANN(48);
	L1RoutingGraph rgraph;
	PathBundle bundle;
}; // APathBundle

TEST(PathBundle, canNotBeConstructedWithEmptyPath)
{
	PathBundle::path_type path;
	ASSERT_ANY_THROW(PathBundle bundle(path));
}

TEST(PathBundle, canBeDefaultConstructed)
{
	PathBundle bundle;
	ASSERT_TRUE(bundle.empty());
}

TEST_F(APathBundle, doesNotAcceptEmptyPath)
{
	ASSERT_ANY_THROW(bundle.add(PathBundle::path_type()));
}

TEST_F(APathBundle, doesNotAcceptPathsWithDifferentSource)
{
	PathBundle::path_type path{rgraph[hicann][VLineOnHICANN(39)],
	                           rgraph[hicann][HLineOnHICANN(48)]};
	ASSERT_ANY_THROW(bundle.add(path));
}

TEST_F(APathBundle, canBeEmpty)
{
	ASSERT_FALSE(bundle.empty());
}

TEST_F(APathBundle, hasSource)
{
	ASSERT_EQ(rgraph[hicann][source], bundle.source());
}

TEST_F(APathBundle, providesIteratorOverTargets)
{
	PathBundle::path_type path{rgraph[hicann][source], rgraph[hicann][VLineOnHICANN(39)],
	                           rgraph[hicann][HLineOnHICANN(49)]};
	ASSERT_NO_THROW(bundle.add(path));
	std::set<PathBundle::vertex_descriptor> targets;
	for (auto vertex : bundle.targets()) {
		targets.insert(vertex);
	}
	EXPECT_EQ(2, targets.size());
	std::set<PathBundle::vertex_descriptor> reference{rgraph[hicann][VLineOnHICANN(39)],
	                                                  rgraph[hicann][HLineOnHICANN(49)]};
	ASSERT_TRUE(std::equal(reference.begin(), reference.end(), targets.begin()));
}

TEST_F(APathBundle, canBeConstructedFromPaths)
{
	ASSERT_NO_THROW(PathBundle bundle_(bundle.paths()));
}

TEST_F(APathBundle, doesNotAcceptEmptyPaths)
{
	auto paths = bundle.paths();
	paths.emplace_back();
	ASSERT_ANY_THROW(PathBundle bundle_(paths));
}

TEST_F(APathBundle, doesNotAcceptPathsWithDifferentSources)
{
	auto paths = bundle.paths();
	paths.push_back(
	    PathBundle::path_type{rgraph[hicann][VLineOnHICANN(39)],
	                          rgraph[hicann][HLineOnHICANN(48)]});
	ASSERT_ANY_THROW(PathBundle bundle_(paths));
}

} // namespace routing
} // namespace marocco
