#include "hal/Coordinate/HMFGeometry.h"
#include "test/common.h"
#include "marocco/routing/SynapseManager.h"

using namespace HMF::Coordinate;
using HMF::HICANN::DriverDecoder;

namespace marocco {
namespace routing {

TEST(SynapseManager, relative_reduction_RequiredEqualAvailable)
{
	std::map<size_t, size_t> required;
	std::map<size_t, size_t> priority;
	required[0] = 2;
	priority[0] = 2;

	required[1] = 3;
	priority[1] = 5;

	size_t available = 5;

	std::map<size_t, size_t> assigned =
		SynapseManager::relative_reduction(required, priority, available);
	ASSERT_EQ(2, assigned[0]);
	ASSERT_EQ(3, assigned[1]);
}

TEST(SynapseManager, relative_reduction_LessAvailableThanRequired)
{
	std::map<size_t, size_t> required;
	std::map<size_t, size_t> priority;
	required[0] = 2;
	priority[0] = 2;

	required[1] = 3;
	priority[1] = 5;

	size_t available = 3;

	std::map<size_t, size_t> assigned =
		SynapseManager::relative_reduction(required, priority, available);
	ASSERT_EQ(1, assigned[0]);
	ASSERT_EQ(2, assigned[1]);
}

TEST(SynapseManager, relative_reduction_AtLeastOne)
{
	std::map<size_t, size_t> required;
	std::map<size_t, size_t> priority;
	required[0] = 1;
	priority[0] = 1;

	required[1] = 2;
	priority[1] = 50;

	size_t available = 2;

	std::map<size_t, size_t> assigned =
		SynapseManager::relative_reduction(required, priority, available);
	ASSERT_EQ(1, assigned[0]);
	ASSERT_EQ(1, assigned[1]);
}

TEST(SynapseManager, relative_reduction_MoreAssignedThanAvailable)
{
	std::map<size_t, size_t> required;
	std::map<size_t, size_t> priority;
	required[0] = 1;
	priority[0] = 1;

	required[1] = 1;
	priority[1] = 2;

	required[2] = 1;
	priority[2] = 3;

	size_t available = 2;

	std::map<size_t, size_t> assigned =
		SynapseManager::relative_reduction(required, priority, available);
	ASSERT_EQ(0, assigned[0]);
	ASSERT_EQ(1, assigned[1]);
	ASSERT_EQ(1, assigned[2]);
}

TEST(SynapseManager, relative_reduction_LessAssignedThanAvailable)
{
	std::map<size_t, size_t> required;
	std::map<size_t, size_t> priority;
	required[0] = 1;
	priority[0] = 4;

	required[1] = 2;
	priority[1] = 5;

	required[2] = 2;
	priority[2] = 6;

	size_t available = 4;

	// rel[0] = 4/15*4 = 16/15;
	// rel[1] = 5/15*4 = 1.333;
	// rel[2] = 6/15*4 = 24/15;

	std::map<size_t, size_t> assigned =
		SynapseManager::relative_reduction(required, priority, available);
	ASSERT_EQ(1, assigned[0]);
	ASSERT_EQ(1, assigned[1]);
	ASSERT_EQ(2, assigned[2]);
}

TEST(SynapseManager, SynapseStepper)
{
	DriverDecoder decoder(0);
	STPMode stp(STPMode::off);

	SynapseManager::Assignment assignment;
	Decoder_STP dec_stp(decoder, stp);
	std::map<Side_Parity, std::vector<SynapseColumnOnHICANN> > side_parity_map;

	// simple stuff:
	// Neuron of 4x2=8 denmems, with standard left right mapping.
	// 1 driver: -> 4 half rows assigned, -> 8 synapses.
	side_parity_map[Side_Parity(geometry::left, Parity::even)] = {SynapseColumnOnHICANN(0),
																  SynapseColumnOnHICANN(2)};
	assignment[Side_Parity_Decoder_STP(geometry::left, Parity::even, decoder, stp)] = {
		SynapseRowOnHICANN(0), SynapseRowOnHICANN(1)};

	side_parity_map[Side_Parity(geometry::left, Parity::odd)] = {SynapseColumnOnHICANN(1),
																 SynapseColumnOnHICANN(3)};
	assignment[Side_Parity_Decoder_STP(geometry::left, Parity::odd, decoder, stp)] = {
		SynapseRowOnHICANN(0), SynapseRowOnHICANN(1)};

	SynapseManager::SynapseStepper stepper(assignment, dec_stp, side_parity_map);

	std::set<SynapseOnHICANN> actual_synapses;
	while (stepper.has_synapses())
		actual_synapses.insert(stepper.get());
	ASSERT_EQ(8, actual_synapses.size());

	ASSERT_THROW(stepper.get(), std::runtime_error);

	std::set<SynapseOnHICANN> expected_synapses;
	for (size_t i = 0; i < 4; ++i)
		for (size_t j = 0; j < 2; ++j)
			expected_synapses.insert(
				SynapseOnHICANN(SynapseRowOnHICANN(j), SynapseColumnOnHICANN(i)));

	ASSERT_EQ(expected_synapses, actual_synapses);
}

} // namespace routing
} // namespace marocco
