#include "test/common.h"

#include <iostream>

#include "marocco/routing/Route.h"

namespace marocco {
namespace routing {

TEST(Route, Base)
{
	routing_graph graph;

	Route route({}, {});
	ASSERT_THROW(route.check(graph), std::runtime_error);
}

} // routing
} // marocco
