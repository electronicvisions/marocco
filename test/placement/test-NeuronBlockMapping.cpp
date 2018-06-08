#include "test/common.h"
#include "marocco/placement/NeuronBlockMapping.h"

using namespace HMF::Coordinate;

namespace marocco {
namespace placement {

TEST(NeuronBlockMapping, Basic)
{
	NeuronBlockMapping mapping;

	ASSERT_EQ(NeuronBlockOnHICANN::end * 64, mapping.available());

	for (size_t ii=0; ii<NeuronBlockOnHICANN::end; ++ii) {
		ASSERT_EQ(64, mapping[NeuronBlockOnHICANN(ii)].available());
	}
}

} // placement
} // marocco
