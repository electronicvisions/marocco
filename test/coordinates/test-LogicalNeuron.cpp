#include <gtest/gtest.h>

#include <algorithm>
#include <iterator>
#include <vector>

#include "hal/Coordinate/HMFGeometry.h"
#include "marocco/coordinates/LogicalNeuron.h"

namespace marocco {

using namespace HMF::Coordinate;

TEST(LogicalNeuron, checksOverlap)
{
	typedef NeuronOnNeuronBlock N;
	NeuronBlockOnWafer const nb{};

	EXPECT_ANY_THROW({ LogicalNeuron::on(nb).add(N(X(0), Y(0)), 3).add(N(X(1), Y(0)), 3).done(); })
		<< "Second chunk lies within first chunk.";

	EXPECT_ANY_THROW({ LogicalNeuron::on(nb).add(N(X(0), Y(0)), 3).add(N(X(0), Y(0)), 2).done(); })
		<< "Multiple chunks start at same offset.";

	EXPECT_ANY_THROW({ LogicalNeuron::on(nb).add(N(X(N::x_type::max - 5), Y(0)), 10).done(); })
	    << "Chunk exceeds neuron block bounds.";
}

TEST(LogicalNeuron, checksSize)
{
	typedef NeuronOnNeuronBlock N;
	NeuronBlockOnWafer const nb{};

	EXPECT_ANY_THROW({ LogicalNeuron::on(nb).done(); })
		<< "No chunks.";

	EXPECT_ANY_THROW({ LogicalNeuron::on(nb).add(N(X(0), Y(0)), 3).add(N(X(5), Y(0)), 0).done(); })
		<< "Second chunk has size zero.";
}

TEST(LogicalNeuron, checksConnectedness)
{
	typedef NeuronOnNeuronBlock N;
	NeuronBlockOnWafer const nb{};

	EXPECT_ANY_THROW({
		// |###  ###
		// |  ##
		LogicalNeuron::on(nb)
		    .add(N(X(0), Y(0)), 3)
		    .add(N(X(5), Y(0)), 3)
		    .add(N(X(2), Y(1)), 2)
		    .done();
	});

	EXPECT_ANY_THROW({
		// |  ##
		// |###  ###
		LogicalNeuron::on(nb)
		    .add(N(X(0), Y(1)), 3)
		    .add(N(X(5), Y(1)), 3)
		    .add(N(X(2), Y(0)), 2)
		    .done();
	});

	EXPECT_ANY_THROW({
		// |###  ###
		// |   ###
		LogicalNeuron::on(nb)
		    .add(N(X(0), Y(0)), 3)
		    .add(N(X(5), Y(0)), 3)
		    .add(N(X(3), Y(1)), 3)
		    .done();
	});

	EXPECT_ANY_THROW({
		// |   ###
		// |###  ###
		LogicalNeuron::on(nb)
		    .add(N(X(0), Y(1)), 3)
		    .add(N(X(5), Y(1)), 3)
		    .add(N(X(3), Y(0)), 3)
		    .done();
	});

	EXPECT_ANY_THROW({
		// |###
		// |   ###
		LogicalNeuron::on(nb).add(N(X(0), Y(0)), 3).add(N(X(3), Y(1)), 3).done();
	});

	EXPECT_ANY_THROW({
		// |   ###
		// |###
		LogicalNeuron::on(nb).add(N(X(0), Y(1)), 3).add(N(X(3), Y(0)), 3).done();
	});

	EXPECT_NO_THROW({
		// |###
		// |  ###
		LogicalNeuron::on(nb).add(N(X(0), Y(0)), 3).add(N(X(2), Y(1)), 3).done();
	});

	EXPECT_NO_THROW({
		// |  ###
		// |###
		LogicalNeuron::on(nb).add(N(X(0), Y(1)), 3).add(N(X(2), Y(0)), 3).done();
	});

	EXPECT_NO_THROW({
		// |###  ###
		// |  ####
		LogicalNeuron::on(nb)
		    .add(N(X(0), Y(0)), 3)
		    .add(N(X(5), Y(0)), 3)
		    .add(N(X(2), Y(1)), 4)
		    .done();
	});

	EXPECT_NO_THROW({
		// |  ####
		// |###  ###
		LogicalNeuron::on(nb)
		    .add(N(X(0), Y(1)), 3)
		    .add(N(X(5), Y(1)), 3)
		    .add(N(X(2), Y(0)), 4)
		    .done();
	});

	EXPECT_NO_THROW({
		// |########
		// |  ####
		LogicalNeuron::on(nb).add(N(X(0), Y(0)), 8).add(N(X(2), Y(1)), 4).done();
	});

	EXPECT_NO_THROW({
		// |  ####
		// |########
		LogicalNeuron::on(nb).add(N(X(0), Y(1)), 8).add(N(X(2), Y(0)), 4).done();
	});

	EXPECT_NO_THROW({
		// |## ### ###
		// | ### ###
		LogicalNeuron::on(nb)
		    .add(N(X(0), Y(0)), 2)
		    .add(N(X(1), Y(1)), 3)
		    .add(N(X(3), Y(0)), 3)
		    .add(N(X(5), Y(1)), 3)
		    .add(N(X(7), Y(0)), 3)
		    .done();
	});

	EXPECT_NO_THROW({
		// | ### ###
		// |## ### ###
		LogicalNeuron::on(nb)
		    .add(N(X(0), Y(1)), 2)
		    .add(N(X(1), Y(0)), 3)
		    .add(N(X(3), Y(1)), 3)
		    .add(N(X(5), Y(0)), 3)
		    .add(N(X(7), Y(1)), 3)
		    .done();
	});
}

TEST(LogicalNeuron, backInserter)
{
	typedef NeuronOnNeuronBlock N;
	NeuronBlockOnWafer const nb{};
	auto builder = LogicalNeuron::on(nb);
	std::vector<LogicalNeuron::chunk_type> chunks{std::make_pair(N(X(0), Y(0)), 8),
	                                              std::make_pair(N(X(2), Y(1)), 4)};
	std::copy(chunks.begin(), chunks.end(), std::back_inserter(builder));
	auto nrn = builder.done();
	EXPECT_EQ(12, nrn.size());
}

TEST(LogicalNeuron, canBeIterated)
{
	NeuronBlockOnWafer const nb{};
	auto logical_neuron = LogicalNeuron::on(nb)
		.add(NeuronOnNeuronBlock(X(0), Y(0)), 3)
		.add(NeuronOnNeuronBlock(X(5), Y(0)), 3)
		.add(NeuronOnNeuronBlock(X(2), Y(1)), 4)
		.done();
	std::vector<NeuronOnNeuronBlock> neurons;
	neurons.reserve(logical_neuron.size());
	for (NeuronOnWafer const nrn : logical_neuron) {
		EXPECT_EQ(nb, nrn.toNeuronBlockOnWafer());
		neurons.push_back(nrn.toNeuronOnNeuronBlock());
	}
	ASSERT_EQ(logical_neuron.size(), neurons.size());
	std::vector<NeuronOnNeuronBlock> reference{
	    NeuronOnNeuronBlock(X(0), Y(0)), NeuronOnNeuronBlock(X(1), Y(0)),
	    NeuronOnNeuronBlock(X(2), Y(0)), NeuronOnNeuronBlock(X(5), Y(0)),
	    NeuronOnNeuronBlock(X(6), Y(0)), NeuronOnNeuronBlock(X(7), Y(0)),
	    NeuronOnNeuronBlock(X(2), Y(1)), NeuronOnNeuronBlock(X(3), Y(1)),
	    NeuronOnNeuronBlock(X(4), Y(1)), NeuronOnNeuronBlock(X(5), Y(1))};
	ASSERT_EQ(reference.size(), neurons.size());
	ASSERT_TRUE(std::equal(reference.begin(), reference.end(), neurons.begin()));
}

TEST(LogicalNeuron, hasFrontAndBack)
{
	EXPECT_ANY_THROW(LogicalNeuron::external(42).front());
	EXPECT_ANY_THROW(LogicalNeuron::external(42).back());

	NeuronBlockOnWafer const nb{};
	auto const logical_neuron = LogicalNeuron::on(nb)
		.add(NeuronOnNeuronBlock(X(0), Y(0)), 3)
		.add(NeuronOnNeuronBlock(X(5), Y(0)), 3)
		.add(NeuronOnNeuronBlock(X(2), Y(1)), 4)
		.done();

	EXPECT_EQ(NeuronOnNeuronBlock(X(0), Y(0)).toNeuronOnWafer(nb), logical_neuron.front());
	EXPECT_EQ(NeuronOnNeuronBlock(X(5), Y(1)).toNeuronOnWafer(nb), logical_neuron.back());
}

TEST(LogicalNeuron, canBeCheckedForRectangularity)
{
	typedef NeuronOnNeuronBlock N;
	NeuronBlockOnWafer const nb{};

	auto nrn = LogicalNeuron::external(42);
	EXPECT_ANY_THROW(nrn.is_rectangular());

	// |###
	// |  ###
	nrn = LogicalNeuron::on(nb).add(N(X(0), Y(0)), 3).add(N(X(2), Y(1)), 3).done();
	EXPECT_FALSE(nrn.is_rectangular());

	// |  ###
	// |###
	nrn = LogicalNeuron::on(nb).add(N(X(0), Y(1)), 3).add(N(X(2), Y(0)), 3).done();
	EXPECT_FALSE(nrn.is_rectangular());

	// |###  ###
	// |  ####
	nrn = LogicalNeuron::on(nb)
		.add(N(X(0), Y(0)), 3)
		.add(N(X(5), Y(0)), 3)
		.add(N(X(2), Y(1)), 4)
		.done();
	EXPECT_FALSE(nrn.is_rectangular());

	// |  ####
	// |###  ###
	nrn = LogicalNeuron::on(nb)
		.add(N(X(0), Y(1)), 3)
		.add(N(X(5), Y(1)), 3)
		.add(N(X(2), Y(0)), 4)
		.done();
	EXPECT_FALSE(nrn.is_rectangular());

	// |########
	// |  ####
	nrn = LogicalNeuron::on(nb).add(N(X(0), Y(0)), 8).add(N(X(2), Y(1)), 4).done();
	EXPECT_FALSE(nrn.is_rectangular());

	// |  ####
	// |########
	nrn = LogicalNeuron::on(nb).add(N(X(0), Y(1)), 8).add(N(X(2), Y(0)), 4).done();
	EXPECT_FALSE(nrn.is_rectangular());

	// |## ### ###
	// | ### ###
	nrn = LogicalNeuron::on(nb)
		.add(N(X(0), Y(0)), 2)
		.add(N(X(1), Y(1)), 3)
		.add(N(X(3), Y(0)), 3)
		.add(N(X(5), Y(1)), 3)
		.add(N(X(7), Y(0)), 3)
		.done();
	EXPECT_FALSE(nrn.is_rectangular());

	// | ### ###
	// |## ### ###
	nrn = LogicalNeuron::on(nb)
		.add(N(X(0), Y(1)), 2)
		.add(N(X(1), Y(0)), 3)
		.add(N(X(3), Y(1)), 3)
		.add(N(X(5), Y(0)), 3)
		.add(N(X(7), Y(1)), 3)
		.done();
	EXPECT_FALSE(nrn.is_rectangular());

	// |  ####
	// |  ####
	nrn = LogicalNeuron::on(nb)
		.add(N(X(2), Y(0)), 4)
		.add(N(X(2), Y(1)), 4)
		.done();
	EXPECT_TRUE(nrn.is_rectangular());

	// |  #
	// |
	nrn = LogicalNeuron::on(nb)
		.add(N(X(2), Y(0)), 1)
		.done();
	EXPECT_TRUE(nrn.is_rectangular());

	// |  ####
	// |
	nrn = LogicalNeuron::on(nb)
		.add(N(X(2), Y(0)), 4)
		.done();
	EXPECT_TRUE(nrn.is_rectangular());

	// |
	// |  #
	nrn = LogicalNeuron::on(nb)
		.add(N(X(2), Y(1)), 1)
		.done();
	EXPECT_TRUE(nrn.is_rectangular());

	// |
	// |  ####
	nrn = LogicalNeuron::on(nb)
		.add(N(X(2), Y(1)), 4)
		.done();
	EXPECT_TRUE(nrn.is_rectangular());
}

TEST(LogicalNeuron, sharesDenmemsWith)
{
	typedef NeuronOnNeuronBlock N;
	NeuronBlockOnWafer nb;

	auto nrn_a = LogicalNeuron::external(42);
	auto nrn_b = LogicalNeuron::external(45);
	EXPECT_ANY_THROW(nrn_a.shares_denmems_with(nrn_b));
	EXPECT_ANY_THROW(nrn_b.shares_denmems_with(nrn_a));

	// |###
	// |  ###
	nrn_a = LogicalNeuron::on(nb).add(N(X(0), Y(0)), 3).add(N(X(2), Y(1)), 3).done();
	EXPECT_ANY_THROW(nrn_a.shares_denmems_with(nrn_b));
	EXPECT_ANY_THROW(nrn_b.shares_denmems_with(nrn_a));

	// |  ###
	// |###
	nrn_b = LogicalNeuron::on(nb).add(N(X(0), Y(1)), 3).add(N(X(2), Y(0)), 3).done();
	EXPECT_TRUE(nrn_a.shares_denmems_with(nrn_a));
	EXPECT_TRUE(nrn_b.shares_denmems_with(nrn_b));
	EXPECT_TRUE(nrn_a.shares_denmems_with(nrn_b));
	EXPECT_TRUE(nrn_b.shares_denmems_with(nrn_a));

	// |###  ###
	// |  ####
	nrn_a = LogicalNeuron::on(nb)
		.add(N(X(0), Y(0)), 3)
		.add(N(X(5), Y(0)), 3)
		.add(N(X(2), Y(1)), 4)
		.done();
	EXPECT_TRUE(nrn_a.shares_denmems_with(nrn_a));
	EXPECT_TRUE(nrn_b.shares_denmems_with(nrn_b));
	EXPECT_TRUE(nrn_a.shares_denmems_with(nrn_b));
	EXPECT_TRUE(nrn_b.shares_denmems_with(nrn_a));

	// |   ##
	// |
	nrn_b = LogicalNeuron::on(nb)
		.add(N(X(3), Y(0)), 2)
		.done();
	EXPECT_TRUE(nrn_a.shares_denmems_with(nrn_a));
	EXPECT_TRUE(nrn_b.shares_denmems_with(nrn_b));
	EXPECT_FALSE(nrn_a.shares_denmems_with(nrn_b));
	EXPECT_FALSE(nrn_b.shares_denmems_with(nrn_a));

	// |    ##
	// |
	nrn_a = LogicalNeuron::on(nb)
		.add(N(X(4), Y(0)), 2)
		.done();
	EXPECT_TRUE(nrn_a.shares_denmems_with(nrn_a));
	EXPECT_TRUE(nrn_b.shares_denmems_with(nrn_b));
	EXPECT_TRUE(nrn_a.shares_denmems_with(nrn_b));
	EXPECT_TRUE(nrn_b.shares_denmems_with(nrn_a));

	// |  ##
	// |
	nrn_b = LogicalNeuron::on(nb)
		.add(N(X(2), Y(0)), 2)
		.done();
	EXPECT_TRUE(nrn_a.shares_denmems_with(nrn_a));
	EXPECT_TRUE(nrn_b.shares_denmems_with(nrn_b));
	EXPECT_FALSE(nrn_a.shares_denmems_with(nrn_b));
	EXPECT_FALSE(nrn_b.shares_denmems_with(nrn_a));

	// |
	// |  ##
	nrn_a = LogicalNeuron::on(nb)
		.add(N(X(2), Y(1)), 2)
		.done();
	EXPECT_TRUE(nrn_a.shares_denmems_with(nrn_a));
	EXPECT_TRUE(nrn_b.shares_denmems_with(nrn_b));
	EXPECT_FALSE(nrn_a.shares_denmems_with(nrn_b));
	EXPECT_FALSE(nrn_b.shares_denmems_with(nrn_a));

	// |
	// |  ##
	nrn_b = LogicalNeuron::on(NeuronBlockOnWafer(NeuronBlockOnHICANN(2)))
		.add(N(X(2), Y(1)), 2)
		.done();
	EXPECT_TRUE(nrn_a.shares_denmems_with(nrn_a));
	EXPECT_TRUE(nrn_b.shares_denmems_with(nrn_b));
	EXPECT_FALSE(nrn_a.shares_denmems_with(nrn_b));
	EXPECT_FALSE(nrn_b.shares_denmems_with(nrn_a));
}

} // namespace marocco
