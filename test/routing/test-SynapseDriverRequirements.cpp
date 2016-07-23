#include "test/common.h"
#include "marocco/routing/SynapseDriverRequirements.h"
#include "hal/Coordinate/iter_all.h"

#include <fstream>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/unordered_map.h>

using namespace HMF::Coordinate;
using HMF::HICANN::DriverDecoder;
using HMF::HICANN::L1Address;

namespace marocco {
namespace routing {

TEST(SynapseDriverRequirements, Base)
{
	std::map<Type_Decoder_STP, size_t> synapses_per_property;

	Type_Decoder_STP sp1(SynapseType::excitatory, DriverDecoder(0), STPMode::off);
	Type_Decoder_STP sp2(SynapseType::excitatory, DriverDecoder(1), STPMode::off);
	Type_Decoder_STP sp3(SynapseType::excitatory, DriverDecoder(2), STPMode::off);
	Type_Decoder_STP sp4(SynapseType::inhibitory, DriverDecoder(3), STPMode::off);

	synapses_per_property[sp1] = 2;
	synapses_per_property[sp2] = 3;
	synapses_per_property[sp3] = 4;
	synapses_per_property[sp4] = 2;

	// assume neuron of 4 denmems -> neuron width = 2, and standard syn input mapping
	std::map<SynapseType, std::map<Side_Parity, size_t> >
		target_synapses_per_parity_and_synaptic_input;
	target_synapses_per_parity_and_synaptic_input[SynapseType::excitatory][Side_Parity(
		geometry::left, Parity::even)] = 1;
	target_synapses_per_parity_and_synaptic_input[SynapseType::excitatory][Side_Parity(
		geometry::left, Parity::odd)] = 1;
	target_synapses_per_parity_and_synaptic_input[SynapseType::inhibitory][Side_Parity(
		geometry::right, Parity::even)] = 1;
	target_synapses_per_parity_and_synaptic_input[SynapseType::inhibitory][Side_Parity(
		geometry::right, Parity::odd)] = 1;

	std::map<Type_Decoder_STP, std::map<Side_Parity, size_t> > required_half_rows =
		SynapseDriverRequirements::count_half_rows_per_input_granularity(
			synapses_per_property, target_synapses_per_parity_and_synaptic_input);

	// there should be:
	// 2 half rows for left, even/odd, DD(0)
	// 3 half rows for left, even/odd, DD(1)
	// 4 half rows for left, even/odd, DD(2)
	// 2 half rows for right, even/odd, DD(3)
	ASSERT_EQ(required_half_rows.size(), 4);
	ASSERT_EQ(required_half_rows[sp1][Side_Parity(geometry::left, Parity::even)], 2);
	ASSERT_EQ(required_half_rows[sp1][Side_Parity(geometry::left, Parity::odd)], 2);

	ASSERT_EQ(required_half_rows[sp2][Side_Parity(geometry::left, Parity::even)], 3);
	ASSERT_EQ(required_half_rows[sp2][Side_Parity(geometry::left, Parity::odd)], 3);

	ASSERT_EQ(required_half_rows[sp3][Side_Parity(geometry::left, Parity::even)], 4);
	ASSERT_EQ(required_half_rows[sp3][Side_Parity(geometry::left, Parity::odd)], 4);

	ASSERT_EQ(required_half_rows[sp4][Side_Parity(geometry::right, Parity::even)], 2);
	ASSERT_EQ(required_half_rows[sp4][Side_Parity(geometry::right, Parity::odd)], 2);


	std::map<Type_Decoder_STP, Side_TriParity> bio_to_hw_assignment;
	std::map<TriParity, std::map<Side_Decoder_STP, size_t> > half_rows_per_parity =
		SynapseDriverRequirements::count_half_rows(required_half_rows, bio_to_hw_assignment);

	ASSERT_EQ(
		half_rows_per_parity[TriParity::any][Side_Decoder_STP(
			geometry::left, DriverDecoder(0), STPMode::off)],
		2);
	ASSERT_EQ(
		half_rows_per_parity[TriParity::any][Side_Decoder_STP(
			geometry::left, DriverDecoder(1), STPMode::off)],
		3);
	ASSERT_EQ(
		half_rows_per_parity[TriParity::any][Side_Decoder_STP(
			geometry::left, DriverDecoder(2), STPMode::off)],
		4);
	ASSERT_EQ(
		half_rows_per_parity[TriParity::any][Side_Decoder_STP(
			geometry::right, DriverDecoder(3), STPMode::off)],
		2);

	ASSERT_EQ(Side_TriParity(geometry::left, TriParity::any), bio_to_hw_assignment[sp1]);
	ASSERT_EQ(Side_TriParity(geometry::left, TriParity::any), bio_to_hw_assignment[sp2]);
	ASSERT_EQ(Side_TriParity(geometry::left, TriParity::any), bio_to_hw_assignment[sp3]);
	ASSERT_EQ(Side_TriParity(geometry::right, TriParity::any), bio_to_hw_assignment[sp4]);
}

TEST(SynapseDriverRequirements, SeveralTargets)
{
	std::vector<std::map<TriParity, std::map<Side_Decoder_STP, size_t> > > half_rows_per_parity;

	Side_Decoder_STP c1(geometry::left, DriverDecoder(0), STPMode::off);
	Side_Decoder_STP c2(geometry::right, DriverDecoder(1), STPMode::off);
	Side_Decoder_STP c3(geometry::right, DriverDecoder(1), STPMode::off);

	{
		std::map<TriParity, std::map<Side_Decoder_STP, size_t> > nrn;
		nrn[TriParity::any][c1] = 2;
		nrn[TriParity::odd][c2] = 3;
		half_rows_per_parity.push_back(nrn);
	}
	{
		std::map<TriParity, std::map<Side_Decoder_STP, size_t> > nrn;
		nrn[TriParity::any][c1] = 4;
		nrn[TriParity::even][c3] = 3;
		half_rows_per_parity.push_back(nrn);
	}
	{
		std::map<TriParity, std::map<Side_Decoder_STP, size_t> > nrn;
		nrn[TriParity::any][c1] = 3;
		nrn[TriParity::even][c3] = 2;
		half_rows_per_parity.push_back(nrn);
	}

	std::map<TriParity, std::map<Side_Decoder_STP, size_t> > required_half_rows =
		SynapseDriverRequirements::count_half_rows_vec(half_rows_per_parity);

	ASSERT_EQ(required_half_rows[TriParity::any][c1], 4);
	ASSERT_EQ(required_half_rows[TriParity::odd][c2], 3);
	ASSERT_EQ(required_half_rows[TriParity::even][c3], 3);
}

TEST(SynapseDriverRequirements, resolve_triparity)
{
	std::map<TriParity, std::map<Side_Decoder_STP, size_t> > required_half_rows;

	Side_Decoder_STP c1(geometry::left, DriverDecoder(0), STPMode::off);
	Side_Decoder_STP c2(geometry::left, DriverDecoder(1), STPMode::off);
	Side_Decoder_STP c3(geometry::left, DriverDecoder(2), STPMode::off);
	Side_Decoder_STP c4(geometry::left, DriverDecoder(3), STPMode::off);

	Side_Decoder_STP c5(geometry::right, DriverDecoder(0), STPMode::off);
	Side_Decoder_STP c6(geometry::right, DriverDecoder(1), STPMode::off);

	required_half_rows[TriParity::even][c1] = 2;
	required_half_rows[TriParity::even][c2] = 3;
	required_half_rows[TriParity::odd][c3] = 4;
	required_half_rows[TriParity::any][c4] = 2;

	required_half_rows[TriParity::odd][c5] = 1;
	required_half_rows[TriParity::odd][c6] = 3;

	std::map<Side_Decoder_STP, std::vector<Parity> > triparity_assignmemt_to_parity;
	std::map<Side_Parity_Decoder_STP, size_t> synrow_hist;

	std::map<Side_Parity_STP, size_t> half_rows_per_parity =
		SynapseDriverRequirements::resolve_triparity(
				required_half_rows, synrow_hist,
				triparity_assignmemt_to_parity);

	// check half rows per parity
	// For c4, which has TriParity::any, we expect that the 2 requested half
	// rows are split evenly among even and odd columns, as there are 2+3=5
	// even half rows requested from c1 and c2, and 4 odd half rows from c3.
	EXPECT_EQ(2+3+1, half_rows_per_parity[Side_Parity_STP(geometry::left,Parity::even, STPMode::off)]);
	EXPECT_EQ(4+1, half_rows_per_parity[Side_Parity_STP(geometry::left,Parity::odd, STPMode::off)]);

	// check assignment from triparity to parity
	// As there are 2+3 even half rows requested from c1 and c2, and 4 odd half
	// rows from c3, we expect that the first half row is assigned to odd
	// columns (fill-up strategy), and the second to an even column.
	ASSERT_EQ(2, triparity_assignmemt_to_parity[c4].size());
	EXPECT_EQ(Parity::odd, triparity_assignmemt_to_parity[c4][0]);
	EXPECT_EQ(Parity::even, triparity_assignmemt_to_parity[c4][1]);

	// check synrow histogram
	Side_Parity_Decoder_STP p1(geometry::left, Parity::even, DriverDecoder(0), STPMode::off);
	EXPECT_EQ(2, synrow_hist[p1]);

	Side_Parity_Decoder_STP p2(geometry::left, Parity::even, DriverDecoder(1), STPMode::off);
	EXPECT_EQ(3, synrow_hist[p2]);

	Side_Parity_Decoder_STP p3(geometry::left, Parity::odd, DriverDecoder(2), STPMode::off);
	EXPECT_EQ(4, synrow_hist[p3]);

	// Again, check that half rows of c4 are split to p4e and p4e
	Side_Parity_Decoder_STP p4o(geometry::left, Parity::odd, DriverDecoder(3), STPMode::off);
	EXPECT_EQ(1, synrow_hist[p4o]);

	Side_Parity_Decoder_STP p4e(geometry::left, Parity::even, DriverDecoder(3), STPMode::off);
	EXPECT_EQ(1, synrow_hist[p4e]);

	Side_Parity_Decoder_STP p5(geometry::right, Parity::odd, DriverDecoder(0), STPMode::off);
	EXPECT_EQ(1, synrow_hist[p5]);

	Side_Parity_Decoder_STP p6(geometry::right, Parity::odd, DriverDecoder(1), STPMode::off);
	EXPECT_EQ(3, synrow_hist[p6]);
}

TEST(SynapseDriverRequirements, count_rows_per_side)
{
	std::map<Side_Parity_STP, size_t> required_half_rows_per_parity;

	Side_Parity_STP c1(geometry::left, Parity::even, STPMode::off);
	Side_Parity_STP c2(geometry::left, Parity::odd, STPMode::off);
	Side_Parity_STP c3(geometry::right, Parity::even, STPMode::off);
	Side_Parity_STP c4(geometry::right, Parity::odd, STPMode::off);
	Side_Parity_STP c5(geometry::left, Parity::even, STPMode::depression);
	Side_Parity_STP c6(geometry::left, Parity::odd, STPMode::depression);
	Side_Parity_STP c7(geometry::left, Parity::even, STPMode::facilitation);

	// even is higher
	required_half_rows_per_parity[c1] = 4;
	required_half_rows_per_parity[c2] = 2;
	// odd is higher
	required_half_rows_per_parity[c3] = 3;
	required_half_rows_per_parity[c4] = 5;
	// equal
	required_half_rows_per_parity[c5] = 2;
	required_half_rows_per_parity[c6] = 2;
	// only one of both
	required_half_rows_per_parity[c7] = 1;

	std::map<Side_STP, size_t> rows_per_side =
		SynapseDriverRequirements::count_rows_per_side(
				required_half_rows_per_parity);

	EXPECT_EQ( 4, rows_per_side[ Side_STP(geometry::left, STPMode::off) ]);
	EXPECT_EQ( 5, rows_per_side[ Side_STP(geometry::right, STPMode::off) ]);
	EXPECT_EQ( 2, rows_per_side[ Side_STP(geometry::left, STPMode::depression) ]);
	EXPECT_EQ( 1, rows_per_side[ Side_STP(geometry::left, STPMode::facilitation) ]);
}

TEST(SynapseDriverRequirements, count_drivers)
{
	std::map<Side_STP, size_t> rows_per_side;

	rows_per_side[Side_STP(geometry::left, STPMode::off)] = 2;
	rows_per_side[Side_STP(geometry::right, STPMode::off)] = 3;

	rows_per_side[Side_STP(geometry::left, STPMode::depression)] = 1;
	rows_per_side[Side_STP(geometry::right, STPMode::depression)] = 1;

	rows_per_side[Side_STP(geometry::left, STPMode::facilitation)] = 3;


	std::map<STPMode, size_t> rows_per_stp =
		SynapseDriverRequirements::count_rows_per_STP(rows_per_side);

	ASSERT_EQ(5, rows_per_stp[STPMode::off]);
	ASSERT_EQ(2, rows_per_stp[STPMode::depression]);
	ASSERT_EQ(3, rows_per_stp[STPMode::facilitation]);

	std::map<STPMode, size_t> drivers_per_stp =
		SynapseDriverRequirements::count_drivers_per_STP(rows_per_stp);

	ASSERT_EQ(3, drivers_per_stp[STPMode::off]);
	ASSERT_EQ(1, drivers_per_stp[STPMode::depression]);
	ASSERT_EQ(2, drivers_per_stp[STPMode::facilitation]);

	ASSERT_EQ(6, SynapseDriverRequirements::count_drivers(drivers_per_stp));
}

TEST(SynapseDriverRequirements, TargetSynapsesPerSynapticInputSimple)
{
	SynapseDriverRequirements::NeuronWidth neuron_width;
	results::SynapticInputs synaptic_inputs;

	for (auto nrn : iter_all<NeuronOnHICANN>()) {
		synaptic_inputs[nrn][geometry::left] = SynapseType::excitatory;
		synaptic_inputs[nrn][geometry::right] = SynapseType::inhibitory;
	}

	neuron_width[NeuronOnHICANN(Enum(0))] = 2;
	neuron_width[NeuronOnHICANN(Enum(2))] = 2;
	neuron_width[NeuronOnHICANN(Enum(4))] = 2;
	neuron_width[NeuronOnHICANN(Enum(6))] = 2;

	neuron_width[NeuronOnHICANN(Enum(8))] = 4;
	neuron_width[NeuronOnHICANN(Enum(12))] = 4;

	neuron_width[NeuronOnHICANN(Enum(16))] = 8;

	auto targets = SynapseDriverRequirements::calc_target_synapses_per_synaptic_input_granularity(
		neuron_width, synaptic_inputs);


	ASSERT_EQ(
		1, targets.at(NeuronOnHICANN(Enum(0)))
			   .at(SynapseType::excitatory)
			   .at(Side_Parity(geometry::left, Parity::even)));
	ASSERT_EQ(
		1, targets.at(NeuronOnHICANN(Enum(0)))
			   .at(SynapseType::excitatory)
			   .at(Side_Parity(geometry::left, Parity::odd)));

	ASSERT_EQ(
		1, targets.at(NeuronOnHICANN(Enum(2)))
			   .at(SynapseType::inhibitory)
			   .at(Side_Parity(geometry::right, Parity::even)));
	ASSERT_EQ(
		1, targets.at(NeuronOnHICANN(Enum(2)))
			   .at(SynapseType::inhibitory)
			   .at(Side_Parity(geometry::right, Parity::odd)));

	ASSERT_EQ(
		2, targets.at(NeuronOnHICANN(Enum(8)))
			   .at(SynapseType::excitatory)
			   .at(Side_Parity(geometry::left, Parity::even)));
	ASSERT_EQ(
		2, targets.at(NeuronOnHICANN(Enum(8)))
			   .at(SynapseType::excitatory)
			   .at(Side_Parity(geometry::left, Parity::odd)));

	ASSERT_EQ(
		4, targets.at(NeuronOnHICANN(Enum(16)))
			   .at(SynapseType::inhibitory)
			   .at(Side_Parity(geometry::right, Parity::even)));
	ASSERT_EQ(
		4, targets.at(NeuronOnHICANN(Enum(16)))
			   .at(SynapseType::inhibitory)
			   .at(Side_Parity(geometry::right, Parity::odd)));
}

TEST(SynapseDriverRequirements, TargetSynapsesPerSynapticInputAdvanced)
{
	SynapseDriverRequirements::NeuronWidth neuron_width;
	results::SynapticInputs synaptic_inputs;

	// 3 time constants
	for (auto nrn : iter_all<NeuronOnHICANN>()) {
		if (nrn.x() % 2 == 0) {
			synaptic_inputs[nrn][geometry::left] = SynapseType(0);
			synaptic_inputs[nrn][geometry::right] = SynapseType(1);
		} else {
			synaptic_inputs[nrn][geometry::left] = SynapseType(0);
			synaptic_inputs[nrn][geometry::right] = SynapseType(2);
		}
	}

	neuron_width[NeuronOnHICANN(Enum(0))] = 2;
	neuron_width[NeuronOnHICANN(Enum(2))] = 2;

	neuron_width[NeuronOnHICANN(Enum(4))] = 3;
	neuron_width[NeuronOnHICANN(Enum(7))] = 3;

	auto targets = SynapseDriverRequirements::calc_target_synapses_per_synaptic_input_granularity(
		neuron_width, synaptic_inputs);

	ASSERT_EQ(
		1, targets.at(NeuronOnHICANN(Enum(0)))
			   .at(SynapseType(0))
			   .at(Side_Parity(geometry::left, Parity::even)));
	ASSERT_EQ(
		1, targets.at(NeuronOnHICANN(Enum(0)))
			   .at(SynapseType(0))
			   .at(Side_Parity(geometry::left, Parity::odd)));

	ASSERT_EQ(
		1, targets.at(NeuronOnHICANN(Enum(2)))
			   .at(SynapseType(1))
			   .at(Side_Parity(geometry::right, Parity::even)));
	ASSERT_EQ(
		1, targets.at(NeuronOnHICANN(Enum(2)))
			   .at(SynapseType(2))
			   .at(Side_Parity(geometry::right, Parity::odd)));

	ASSERT_EQ(
		2, targets.at(NeuronOnHICANN(Enum(4)))
			   .at(SynapseType(0))
			   .at(Side_Parity(geometry::left, Parity::even)));
	ASSERT_EQ(
		1, targets.at(NeuronOnHICANN(Enum(4)))
			   .at(SynapseType(0))
			   .at(Side_Parity(geometry::left, Parity::odd)));

	ASSERT_EQ(
		2, targets.at(NeuronOnHICANN(Enum(4)))
			   .at(SynapseType(1))
			   .at(Side_Parity(geometry::right, Parity::even)));
	ASSERT_EQ(
		1, targets.at(NeuronOnHICANN(Enum(4)))
			   .at(SynapseType(2))
			   .at(Side_Parity(geometry::right, Parity::odd)));

	ASSERT_EQ(
		1, targets.at(NeuronOnHICANN(Enum(7)))
			   .at(SynapseType(0))
			   .at(Side_Parity(geometry::left, Parity::even)));
	ASSERT_EQ(
		2, targets.at(NeuronOnHICANN(Enum(7)))
			   .at(SynapseType(0))
			   .at(Side_Parity(geometry::left, Parity::odd)));

	ASSERT_EQ(
		1, targets.at(NeuronOnHICANN(Enum(7)))
			   .at(SynapseType(1))
			   .at(Side_Parity(geometry::right, Parity::even)));
	ASSERT_EQ(
		2, targets.at(NeuronOnHICANN(Enum(7)))
			   .at(SynapseType(2))
			   .at(Side_Parity(geometry::right, Parity::odd)));
}

TEST(SynapseDriverRequirements, _calc)
{
	SynapseDriverRequirements::NeuronWidth neuron_width;
	results::SynapticInputs synaptic_inputs;

	for (auto nrn : iter_all<NeuronOnHICANN>()) {
		synaptic_inputs[nrn][geometry::left] = SynapseType::excitatory;
		synaptic_inputs[nrn][geometry::right] = SynapseType::inhibitory;
	}
	//  NeuronSize = 4 -> neuron_width = 2, 1 row = 2 inputs.
	for (size_t xx = 0; xx < NeuronOnHICANN::x_type::end; xx += 2) {
		neuron_width[NeuronOnHICANN(X(xx), Y(0))] = 2;
	}

	std::unordered_map<HMF::Coordinate::NeuronOnHICANN,
					   SynapseDriverRequirements::Side_Parity_count_per_synapse_type>
		target_synapses_per_synaptic_input_granularity =
			SynapseDriverRequirements::calc_target_synapses_per_synaptic_input_granularity(
				neuron_width, synaptic_inputs);

	SynapseCounts sc;

	sc.add(NeuronOnHICANN(Enum(0)), L1Address(2), SynapseType::excitatory, STPMode::off);
	sc.add(NeuronOnHICANN(Enum(0)), L1Address(3), SynapseType::excitatory, STPMode::off);
	sc.add(NeuronOnHICANN(Enum(0)), L1Address(4), SynapseType::excitatory, STPMode::off);
	sc.add(NeuronOnHICANN(Enum(0)), L1Address(5), SynapseType::excitatory, STPMode::off);
	sc.add(NeuronOnHICANN(Enum(0)), L1Address(6), SynapseType::excitatory, STPMode::off);

	size_t num_drivers, num_synapses;
	std::map<Side_Parity_Decoder_STP, size_t> synapse_histogram;
	std::map<Side_Parity_Decoder_STP, size_t> synrow_histogram;
	std::tie(num_drivers, num_synapses) = SynapseDriverRequirements::_calc(
		sc, target_synapses_per_synaptic_input_granularity, synapse_histogram, synrow_histogram);

	ASSERT_EQ(2, num_drivers);
	ASSERT_EQ(5, num_synapses);
}

TEST(SynapseDriverRequirements, count_synapses_per_hardware_property)
{
	// Test with failing assertion in systemsim-test test hardware neuron size of 6 and 10, etc..
	// (i.e. configurations that are not symmetric)
	// and only 1 synapse needed per neuron

	Type_Decoder_STP bio1(SynapseType(0), DriverDecoder(0), STPMode::off);
	Type_Decoder_STP bio2(SynapseType(1), DriverDecoder(0), STPMode::off);
	Type_Decoder_STP bio3(SynapseType(2), DriverDecoder(0), STPMode::off);

	std::map<Type_Decoder_STP, size_t> bio_property_counts;
	bio_property_counts[bio1] = 1; // only one, so that one even or odd half row is fine.
	bio_property_counts[bio2] = 4;
	bio_property_counts[bio3] = 3;

	// neuron size = 6, mapping is 0 1, 0 2, 0 1
	std::map<SynapseType, std::map<Side_Parity, size_t> >
		target_synapses_per_parity_and_synaptic_input;
	target_synapses_per_parity_and_synaptic_input[SynapseType(0)][Side_Parity(
		geometry::left, Parity::even)] = 2;
	target_synapses_per_parity_and_synaptic_input[SynapseType(0)][Side_Parity(
		geometry::left, Parity::odd)] = 1;
	target_synapses_per_parity_and_synaptic_input[SynapseType(1)][Side_Parity(
		geometry::right, Parity::even)] = 2;
	target_synapses_per_parity_and_synaptic_input[SynapseType(2)][Side_Parity(
		geometry::right, Parity::odd)] = 1;

	std::map<Type_Decoder_STP, Side_TriParity> bio_to_hw_assignment;
	bio_to_hw_assignment[bio1] = Side_TriParity(geometry::left, TriParity::any);
	bio_to_hw_assignment[bio2] = Side_TriParity(geometry::right, TriParity::even);
	bio_to_hw_assignment[bio3] = Side_TriParity(geometry::right, TriParity::odd);

	// synapse row histogram
	std::map<Side_Parity_Decoder_STP, size_t> half_rows_assigned_per_parity;
	// synapse type 1: requires 2 half rows for 4 synapses, as there are 2 target synapses per (right, even) config.
	half_rows_assigned_per_parity[Side_Parity_Decoder_STP(geometry::right, Parity::even, DriverDecoder(0), STPMode::off)] = 2;
	// synapse type 2: requires 3 half rows for 3 synapses on (right, odd) config.
	half_rows_assigned_per_parity[Side_Parity_Decoder_STP(geometry::right, Parity::odd, DriverDecoder(0), STPMode::off)] = 3;
	// synapse type 0: TriParity::any is resolved to even columns, which is prioritized over odd columns
	half_rows_assigned_per_parity[Side_Parity_Decoder_STP(geometry::left, Parity::even, DriverDecoder(0), STPMode::off)] = 1;

	// assignment to parity for TriParity::any requests
	std::map<Side_Decoder_STP, std::vector<Parity> > assignmemt_to_parity;
	// only for synapse type 0. Even columns have priority over odd columns.
	assignmemt_to_parity[ Side_Decoder_STP(geometry::left, DriverDecoder(0), STPMode::off) ].push_back(Parity::even);

	std::map<Side_Parity_Decoder_STP, size_t> hardware_property_counts =
		SynapseDriverRequirements::count_synapses_per_hardware_property(
			bio_property_counts, bio_to_hw_assignment,
			target_synapses_per_parity_and_synaptic_input,
			assignmemt_to_parity, half_rows_assigned_per_parity);

	// check hardware property counts
	EXPECT_EQ(1, hardware_property_counts[ Side_Parity_Decoder_STP(geometry::left, Parity::even, DriverDecoder(0), STPMode::off) ]);
	EXPECT_EQ(4, hardware_property_counts[ Side_Parity_Decoder_STP(geometry::right, Parity::even, DriverDecoder(0), STPMode::off) ]);
	EXPECT_EQ(3, hardware_property_counts[ Side_Parity_Decoder_STP(geometry::right, Parity::odd, DriverDecoder(0), STPMode::off) ]);
}

// helper for next tests
template <typename Key, typename T>
struct map_acc
{
	T operator()(T const& lhs, std::pair<Key, T> const& rhs)
	{
		return lhs + rhs.second;
	}
};

// helper function to count number of required driver from synrow_histogram
// procedure taken from SynapseManager.init().
// needed for next tests
size_t count_drivers_from_synrow_histogram(
		std::map<Side_Parity_Decoder_STP, size_t> const& synrow_histogram
		)
{
	typedef std::map<STPMode,
					 std::map<Side, std::map<Parity, std::map<DriverDecoder, size_t> > > >
		PerDecoderMap;

	PerDecoderMap half_rows;

	for (auto const& item : synrow_histogram) {
		Side side;
		Parity parity;
		DriverDecoder decoder;
		STPMode stp;
		std::tie(side, parity, decoder, stp) = item.first;
		size_t const half_row_count = item.second;

		half_rows[stp][side][parity][decoder] = half_row_count;
	}

	// calc requirements
	size_t r_drivers = 0; // number of required drivers
	for (auto const& stp_item : half_rows) {
		size_t req_rows_per_stp = 0;
		for (auto const& side_item : stp_item.second) {
			// The number of required rows per side is maximum of half rows per parity;
			size_t req_rows_per_side = 0;
			for (auto const& parity_item : side_item.second) {
				size_t req_half_rows = std::accumulate(
					parity_item.second.begin(), parity_item.second.end(), 0,
					map_acc<DriverDecoder, size_t>());
				if (req_half_rows > req_rows_per_side)
					req_rows_per_side = req_half_rows;
			}
			req_rows_per_stp += req_rows_per_side;
		}
		size_t const req_drivers_per_stp =
			size_t(std::ceil(0.5 * req_rows_per_stp)); // 2 rows per driver
		r_drivers += req_drivers_per_stp;
	}
	return r_drivers;
}

/// Minimal example reproducing #2115 with only 2 neurons
/// Setup: neuron size 4 and default synapse target mapping
///
/// Synapses:
/// 1st neuron has 3 synapes from DriverDecoder(0)
/// 2nd neuron has 1 synapse from DriverDecoder(1)
///
/// 1 Driver is sufficient to implement all synapses, which is correctly
/// calcultated by _calc().
/// However, the assignment for bio synapses to hardware half synapse rows
/// fails, because for both neurons we start with even half rows, so that at
/// the end 3 even and 1 odd half row are requested, which implies 2
/// requested drivers (and not 1).
TEST(SynapseDriverRequirements, Issue2115_minimal)
{
	SynapseDriverRequirements::NeuronWidth neuron_width;
	results::SynapticInputs synaptic_inputs;

	for (auto nrn : iter_all<NeuronOnHICANN>()) {
		synaptic_inputs[nrn][geometry::left] = SynapseType::excitatory;
		synaptic_inputs[nrn][geometry::right] = SynapseType::inhibitory;
	}
	//  NeuronSize = 4 -> neuron_width = 2, 1 row = 2 inputs.
	for (size_t xx = 0; xx < NeuronOnHICANN::x_type::end; xx += 2) {
		neuron_width[NeuronOnHICANN(X(xx), Y(0))] = 2;
	}

	std::unordered_map<HMF::Coordinate::NeuronOnHICANN,
		SynapseDriverRequirements::Side_Parity_count_per_synapse_type>
			target_synapses_per_synaptic_input_granularity =
			SynapseDriverRequirements::calc_target_synapses_per_synaptic_input_granularity(
					neuron_width, synaptic_inputs);

	SynapseCounts sc;
	// neuron 0: 3 half rows -> 3 synapses
	sc.add(NeuronOnHICANN(Enum(0)), L1Address(1), SynapseType::excitatory, STPMode::off);
	sc.add(NeuronOnHICANN(Enum(0)), L1Address(2), SynapseType::excitatory, STPMode::off);
	sc.add(NeuronOnHICANN(Enum(0)), L1Address(3), SynapseType::excitatory, STPMode::off);

	// neuron 1: 1 half row -> 1 synapse
	sc.add(NeuronOnHICANN(Enum(2)), L1Address(16), SynapseType::excitatory, STPMode::off);

	size_t num_drivers, num_synapses;
	std::map<Side_Parity_Decoder_STP, size_t> synapse_histogram;
	std::map<Side_Parity_Decoder_STP, size_t> synrow_histogram;
	std::tie(num_drivers, num_synapses) = SynapseDriverRequirements::_calc(
		sc, target_synapses_per_synaptic_input_granularity, synapse_histogram,
		synrow_histogram);

	ASSERT_EQ(1, num_drivers);
	ASSERT_EQ(4, num_synapses);

	// count number of requested drivers from synrow histogram
	size_t r_drivers = count_drivers_from_synrow_histogram(synrow_histogram);

	EXPECT_EQ(num_drivers, r_drivers);
}

} // namespace routing
} // namespace marocco
