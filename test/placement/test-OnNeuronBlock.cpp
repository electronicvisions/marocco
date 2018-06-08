#include "test/common.h"

#include <vector>

#include "marocco/assignment/PopulationSlice.h"
#include "marocco/placement/OnNeuronBlock.h"
#include "marocco/util.h"

using namespace HMF::Coordinate;

namespace marocco {
namespace placement {

class OnNeuronBlockTest : public ::testing::Test {
protected:
	OnNeuronBlock onb;
};

NeuronPlacement make_assignment(size_t size) {
	return {assignment::PopulationSlice{{}, 0 /*offset*/, size}, 2 /*hw_neuron_size*/};
}

TEST_F(OnNeuronBlockTest, empty) {
	ASSERT_TRUE(onb.empty());
	onb.add(make_assignment(1));
	ASSERT_FALSE(onb.empty());
}

TEST_F(OnNeuronBlockTest, available) {
	ASSERT_EQ(64, onb.available());
	onb.add(make_assignment(1));
	ASSERT_EQ(62, onb.available());
	onb.add(make_assignment(1));
	ASSERT_EQ(60, onb.available());
}

TEST_F(OnNeuronBlockTest, IteratesAllPopulations) {
	onb.add(make_assignment(1));
	onb.add(make_assignment(2));
	onb.add(make_assignment(4));

	size_t seen = 0;
	for (auto it = onb.begin(); it != onb.end(); ++it) {
		ASSERT_FALSE(!*it);
		seen |= (*it)->population_slice().size();
	}
	ASSERT_EQ(1 + 2 + 4, seen);
}

TEST_F(OnNeuronBlockTest, DoesNotIterateDefects) {
	for (auto it = onb.begin(); it != onb.end(); ++it) {
		FAIL() << "Empty, should not iterate.";
	}

	onb.add_defect(NeuronOnNeuronBlock(Enum(5)));
	onb.add_defect(NeuronOnNeuronBlock(Enum(2)));

	for (auto it = onb.begin(); it != onb.end(); ++it) {
		FAIL() << "Only defects added, should not iterate.";
	}
}

TEST_F(OnNeuronBlockTest, MayFailToAssign) {
	for (size_t xx = 0; xx < 4; ++xx) {
		ASSERT_TRUE(!onb[NeuronOnNeuronBlock(X(xx), Y(0))]);
		ASSERT_TRUE(!onb[NeuronOnNeuronBlock(X(xx), Y(1))]);

		onb.add_defect(NeuronOnNeuronBlock(X(xx), Y(0)));
		onb.add_defect(NeuronOnNeuronBlock(X(xx), Y(1)));

		ASSERT_TRUE(!onb[NeuronOnNeuronBlock(X(xx), Y(0))]);
		ASSERT_TRUE(!onb[NeuronOnNeuronBlock(X(xx), Y(1))]);
	}

	for (size_t xx = 6; xx < NeuronOnNeuronBlock::x_type::end; ++xx) {
		onb.add_defect(NeuronOnNeuronBlock(X(xx), Y(0)));
		onb.add_defect(NeuronOnNeuronBlock(X(xx), Y(1)));
	}

	/* We have the following assignments (X: defect):
	 * | X | X | X | X |   |   | X | ...
	 * | X | X | X | X |   |   | X | ... */

	ASSERT_TRUE(!onb[NeuronOnNeuronBlock(X(4), Y(0))]);
	ASSERT_TRUE(!onb[NeuronOnNeuronBlock(X(4), Y(1))]);
	ASSERT_TRUE(!onb[NeuronOnNeuronBlock(X(5), Y(0))]);
	ASSERT_TRUE(!onb[NeuronOnNeuronBlock(X(5), Y(1))]);

	ASSERT_EQ(4, onb.available());
	// 3 * 2 hardware neurons do not fit in:
	ASSERT_EQ(onb.end(), onb.add(make_assignment(3)));
	ASSERT_NE(onb.end(), onb.add(make_assignment(1)));

	/* | X | X | X | X | * |   | X | ...
	 * | X | X | X | X | * |   | X | ... */

	ASSERT_FALSE(!onb[NeuronOnNeuronBlock(X(4), Y(0))]);
	ASSERT_FALSE(!onb[NeuronOnNeuronBlock(X(4), Y(1))]);
	ASSERT_TRUE(!onb[NeuronOnNeuronBlock(X(5), Y(0))]);
	ASSERT_TRUE(!onb[NeuronOnNeuronBlock(X(5), Y(1))]);

	ASSERT_EQ(2, onb.available());
}

TEST_F(OnNeuronBlockTest, DoesNotStartAssignmentsInBottomRow) {
	for (size_t xx = 3; xx < NeuronOnNeuronBlock::x_type::end; ++xx) {
		onb.add_defect(NeuronOnNeuronBlock(X(xx), Y(0)));
		onb.add_defect(NeuronOnNeuronBlock(X(xx), Y(1)));
	}

	onb.add_defect(NeuronOnNeuronBlock(X(0), Y(0)));
	onb.add_defect(NeuronOnNeuronBlock(X(1), Y(1)));

	/* We have the following assignments (X: defect):
	 * | X |   |   | X | ...
	 * |   | X |   | X | ... */

	auto it = onb.add(make_assignment(1));
	auto first = *onb.neurons(it).begin();
	ASSERT_EQ(NeuronOnNeuronBlock(X(2), Y(0)), first);

	/* | X |   | * | X | ...
	 * |   | X | * | X | ... */

	// Fails because assignments may only start at top neuron row.
	ASSERT_EQ(onb.end(), onb.add(make_assignment(1)));
}

TEST_F(OnNeuronBlockTest, ReturnsPopulation) {
	onb.add(make_assignment(3));
	ASSERT_EQ(3, onb[NeuronOnNeuronBlock(X(1), Y(0))]->population_slice().size());
}

TEST_F(OnNeuronBlockTest, ReturnsIterator) {
	onb.add(make_assignment(3));
	onb.add(make_assignment(4));
	auto it = onb.begin();
	ASSERT_EQ(it, onb.get(NeuronOnNeuronBlock(X(1), Y(0))));
	++it;
	ASSERT_EQ(it, onb.get(NeuronOnNeuronBlock(X(4), Y(0))));
	ASSERT_EQ(it, onb.get(NeuronOnNeuronBlock(X(5), Y(1))));
}

TEST_F(OnNeuronBlockTest, ReturnsIteratorOnAdd) {
	ASSERT_EQ(onb.begin(), onb.add(make_assignment(3)));
}

TEST_F(OnNeuronBlockTest, AllowsIterationOfPopulationNeurons) {
	onb.add(make_assignment(2));

	auto it = onb.begin();
	ASSERT_FALSE(!*it);
	auto const pl = *it;
	ASSERT_EQ(4, pl->size());

	size_t count = 0;
	for (auto nrn : onb.neurons(it)) {
		++count;
	}

	ASSERT_EQ(4, count);

	std::vector<size_t> neuron_enums{33, 1, 32, 0};

	for (auto nrn : onb.neurons(it)) {
		ASSERT_EQ(neuron_enums.back(), nrn.id());
		neuron_enums.pop_back();
	}
}

} // namespace placement
} // namespace marocco
