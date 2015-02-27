#include "test/common.h"
#include "marocco/routing/SynapseDriverRequirements.h"
#include "hal/Coordinate/iter_all.h"

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

TEST(SynapseDriverRequirements, count_half_rows_per_side)
{
	std::map<TriParity, std::map<Side_Decoder_STP, size_t> > required_half_rows;

	Side_Decoder_STP c1(geometry::left, DriverDecoder(1), STPMode::off);
	Side_Decoder_STP c2(geometry::left, DriverDecoder(2), STPMode::off);
	Side_Decoder_STP c3(geometry::left, DriverDecoder(3), STPMode::off);

	Side_Decoder_STP c4(geometry::right, DriverDecoder(0), STPMode::off);
	Side_Decoder_STP c5(geometry::right, DriverDecoder(1), STPMode::off);

	required_half_rows[TriParity::even][c1] = 2;
	required_half_rows[TriParity::even][c2] = 3;
	required_half_rows[TriParity::even][c3] = 4;

	required_half_rows[TriParity::odd][c4] = 1;
	required_half_rows[TriParity::odd][c5] = 3;


	std::map<TriParity, std::map<Side_STP, size_t> > required_half_rows_per_side =
		SynapseDriverRequirements::count_half_rows_per_side(required_half_rows);

	ASSERT_EQ(
		2 + 3 + 4,
		required_half_rows_per_side[TriParity::even][Side_STP(geometry::left, STPMode::off)]);
	ASSERT_EQ(
		1 + 3,
		required_half_rows_per_side[TriParity::odd][Side_STP(geometry::right, STPMode::off)]);
}

TEST(SynapseDriverRequirements, count_rows_per_side)
{
	std::map<TriParity, std::map<Side_STP, size_t> > required_half_rows_per_side;

	Side_STP c1(geometry::left, STPMode::off);
	Side_STP c2(geometry::right, STPMode::off);
	Side_STP c3(geometry::left, STPMode::depression);
	Side_STP c4(geometry::left, STPMode::facilitation);

	required_half_rows_per_side[TriParity::even][c1] = 2;
	required_half_rows_per_side[TriParity::odd][c1] = 3;
	required_half_rows_per_side[TriParity::any][c1] = 2;

	required_half_rows_per_side[TriParity::even][c2] = 5;
	required_half_rows_per_side[TriParity::odd][c2] = 2;
	required_half_rows_per_side[TriParity::any][c2] = 2;

	required_half_rows_per_side[TriParity::even][c3] = 2;
	required_half_rows_per_side[TriParity::odd][c3] = 3;
	required_half_rows_per_side[TriParity::any][c3] = 3;

	required_half_rows_per_side[TriParity::even][c4] = 5;
	required_half_rows_per_side[TriParity::odd][c4] = 2;
	required_half_rows_per_side[TriParity::any][c4] = 1;

	std::map<Side_STP, std::map<Parity, size_t> > half_rows_assigned_to_parity;
	std::map<Side_STP, size_t> rows_per_side = SynapseDriverRequirements::count_rows_per_side(
		required_half_rows_per_side, half_rows_assigned_to_parity);

	ASSERT_EQ(4, rows_per_side[c1]);
	ASSERT_EQ(5, rows_per_side[c2]);
	ASSERT_EQ(4, rows_per_side[c3]);
	ASSERT_EQ(5, rows_per_side[c4]);

	ASSERT_EQ(4, half_rows_assigned_to_parity[c1][Parity::even]);
	ASSERT_EQ(3, half_rows_assigned_to_parity[c1][Parity::odd]);

	ASSERT_EQ(5, half_rows_assigned_to_parity[c2][Parity::even]);
	ASSERT_EQ(4, half_rows_assigned_to_parity[c2][Parity::odd]);

	ASSERT_EQ(4, half_rows_assigned_to_parity[c3][Parity::even]);
	ASSERT_EQ(4, half_rows_assigned_to_parity[c3][Parity::odd]);

	ASSERT_EQ(5, half_rows_assigned_to_parity[c4][Parity::even]);
	ASSERT_EQ(3, half_rows_assigned_to_parity[c4][Parity::odd]);
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
	SynapseTargetMapping syn_target_mapping;

	for (auto nrn : iter_all<NeuronOnHICANN>()) {
		syn_target_mapping[nrn][geometry::left] = SynapseType::excitatory;
		syn_target_mapping[nrn][geometry::right] = SynapseType::inhibitory;
	}

	neuron_width[NeuronOnHICANN(Enum(0))] = 2;
	neuron_width[NeuronOnHICANN(Enum(2))] = 2;
	neuron_width[NeuronOnHICANN(Enum(4))] = 2;
	neuron_width[NeuronOnHICANN(Enum(6))] = 2;

	neuron_width[NeuronOnHICANN(Enum(8))] = 4;
	neuron_width[NeuronOnHICANN(Enum(12))] = 4;

	neuron_width[NeuronOnHICANN(Enum(16))] = 8;

	auto targets = SynapseDriverRequirements::calc_target_synapses_per_synaptic_input_granularity(
		neuron_width, syn_target_mapping);


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
	SynapseTargetMapping syn_target_mapping;

	// 3 time constants
	for (auto nrn : iter_all<NeuronOnHICANN>()) {
		if (nrn.x() % 2 == 0) {
			syn_target_mapping[nrn][geometry::left] = SynapseType(0);
			syn_target_mapping[nrn][geometry::right] = SynapseType(1);
		} else {
			syn_target_mapping[nrn][geometry::left] = SynapseType(0);
			syn_target_mapping[nrn][geometry::right] = SynapseType(2);
		}
	}

	neuron_width[NeuronOnHICANN(Enum(0))] = 2;
	neuron_width[NeuronOnHICANN(Enum(2))] = 2;

	neuron_width[NeuronOnHICANN(Enum(4))] = 3;
	neuron_width[NeuronOnHICANN(Enum(7))] = 3;

	auto targets = SynapseDriverRequirements::calc_target_synapses_per_synaptic_input_granularity(
		neuron_width, syn_target_mapping);

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
	SynapseTargetMapping syn_target_mapping;

	for (auto nrn : iter_all<NeuronOnHICANN>()) {
		syn_target_mapping[nrn][geometry::left] = SynapseType::excitatory;
		syn_target_mapping[nrn][geometry::right] = SynapseType::inhibitory;
	}
	//  NeuronSize = 4 -> neuron_width = 2, 1 row = 2 inputs.
	for (size_t xx = 0; xx < NeuronOnHICANN::x_type::end; xx += 2) {
		neuron_width[NeuronOnHICANN(X(xx), Y(0))] = 2;
	}

	std::unordered_map<HMF::Coordinate::NeuronOnHICANN,
					   SynapseDriverRequirements::Side_Parity_count_per_synapse_type>
		target_synapses_per_synaptic_input_granularity =
			SynapseDriverRequirements::calc_target_synapses_per_synaptic_input_granularity(
				neuron_width, syn_target_mapping);

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

	std::map<Type_Decoder_STP, size_t> bio_property_counts;
	bio_property_counts[Type_Decoder_STP(SynapseType(0), DriverDecoder(0), STPMode::off)] =
		1; // only one, so that one even or odd half row is fine.
	bio_property_counts[Type_Decoder_STP(SynapseType(1), DriverDecoder(0), STPMode::off)] = 4;
	bio_property_counts[Type_Decoder_STP(SynapseType(2), DriverDecoder(0), STPMode::off)] = 3;

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
	bio_to_hw_assignment[Type_Decoder_STP(SynapseType(0), DriverDecoder(0), STPMode::off)] =
		Side_TriParity(geometry::left, TriParity::any);
	bio_to_hw_assignment[Type_Decoder_STP(SynapseType(1), DriverDecoder(0), STPMode::off)] =
		Side_TriParity(geometry::right, TriParity::even);
	bio_to_hw_assignment[Type_Decoder_STP(SynapseType(2), DriverDecoder(0), STPMode::off)] =
		Side_TriParity(geometry::right, TriParity::odd);

	std::map<Side_STP, size_t> assigned_rows_per_side_stp;
	assigned_rows_per_side_stp[Side_STP(geometry::left, STPMode::off)] = 1;
	assigned_rows_per_side_stp[Side_STP(geometry::right, STPMode::off)] = 3;

	std::map<Side_Parity_Decoder_STP, size_t> half_rows_assigned_per_parity;
	std::map<Type_Decoder_STP, std::vector<Side_Parity> > assigned_side_parity;

	std::map<Side_Parity_Decoder_STP, size_t> hardware_property_counts =
		SynapseDriverRequirements::count_synapses_per_hardware_property(
			// in
			bio_property_counts, assigned_rows_per_side_stp, bio_to_hw_assignment,
			target_synapses_per_parity_and_synaptic_input,
			// out
			half_rows_assigned_per_parity, assigned_side_parity);
}


} // namespace routing
} // namespace marocco
