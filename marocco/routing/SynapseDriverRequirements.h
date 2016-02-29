#pragma once

#include <array>
#include <unordered_map>
#include <tuple>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/export.hpp>

#include "hal/Coordinate/HMFGeometry.h"
#include "hal/HICANN/L1Address.h"
#include "marocco/routing/SynapseTargetMapping.h"
#include "marocco/placement/Result.h"
#include "marocco/routing/LocalRoute.h"
#include "marocco/routing/STPMode.h"
#include "marocco/routing/SynapseType.h"
#include "marocco/test.h"

namespace marocco {
namespace routing {

enum class Parity
{
	even = 0,
	odd = 1
};
std::ostream& operator<<(std::ostream& os, Parity p);

/// Parity with the option that both columns are fine
/// name inspired by boost::tribool
enum class TriParity
{
	even = 0,
	odd = 1,
	any = 2
};
std::ostream& operator<<(std::ostream& os, TriParity p);

typedef std::tuple<SynapseType, HMF::HICANN::DriverDecoder, STPMode> Type_Decoder_STP;

typedef std::tuple<HMF::Coordinate::Side, Parity, HMF::HICANN::DriverDecoder, STPMode>
	Side_Parity_Decoder_STP;

typedef std::tuple<HMF::Coordinate::Side, Parity> Side_Parity;

typedef std::tuple<HMF::Coordinate::Side, TriParity> Side_TriParity;

typedef std::tuple<HMF::Coordinate::Side, Parity, STPMode> Side_Parity_STP;

typedef std::tuple<HMF::Coordinate::Side, HMF::HICANN::DriverDecoder, STPMode> Side_Decoder_STP;

typedef std::tuple<HMF::HICANN::DriverDecoder, STPMode> Decoder_STP;

typedef std::tuple<HMF::Coordinate::Side, STPMode> Side_STP;


inline Side_STP to_Side_STP(Side_Decoder_STP const& rhs)
{
	return Side_STP(std::get<0>(rhs), std::get<2>(rhs));
}

inline Parity to_Parity(size_t i)
{
	return i % 2 ? Parity::odd : Parity::even;
}


/// Holds counts of synapses for each combination of 2MSB pattern, bio synapse_type, STPMode for
/// hardware neurons on a HICANN.
///
/// @note: although this class stores counts for single hardware neurons, currently only
///        the top left neuron of the compound neurons are used
class SynapseCounts
{
public:
	typedef std::map<Type_Decoder_STP, size_t> property_counts;
	typedef std::unordered_map<HMF::Coordinate::NeuronOnHICANN, property_counts> mapped_type;


	/// increase synapse count for target neuron 'p', for DriverDecoderMask
	/// of source 'addr', target 'type', and STP mode 'stp'.
	///
	/// @param p target neuron described by coordinate of top left neuron of compound neuron
	/// @param addr 6 bit Layer 1 Address of pre synaptic neuron
	/// @param type target of the synapse, a.k.a the synapse type
	/// @param stp STP mode of the synapse
	void
	add(HMF::Coordinate::NeuronOnHICANN const& p,
		HMF::HICANN::L1Address const& addr,
		SynapseType const& type,
		STPMode const& stp)
	{
		mCounts[p][Type_Decoder_STP(type, addr.getDriverDecoderMask(), stp)] += 1;
	}

	mapped_type const& get_counts() const
	{
		return mCounts;
	}

private:
	/// map from hardware neuron coordinate to property counts
	mapped_type mCounts;

	/// serialization
	friend class boost::serialization::access;
	template<typename Archiver>
	void serialize(Archiver& ar, unsigned int const);
};


/** class to calculate the number of required synapse drivers for incoming L1
 * routes in order to realize all synapses to neurons on the HICANN.
 *
 * Incoming synaptic connections are defined by certain ``bio properties'',
 * namely a 6 bit L1 Address of the source neuron, the target neuron, and
 * synapse parameters like the synaptic weight, the short term plasticity
 * parameters, and the synapse type (e.g. excitatory or inhibitory).
 * In contrast, many of the hardware synapse parameters can not be set per
 * synapse but are shared per synapse row, synapse driver or synapse column.
 * In fact, the following constraints must be considered:
 *  - The short term plasticity settings are set once per synapse driver.
 *  - Each synapse decodes the 4 LSB of the L1 address, the 2 MSB are set per
 *    half synapse row. A half synapse row contains all synapses of a row with
 *    the same column parity.
 *  - All synapses of one row connect to either the left or the right synaptic
 *    input circuit of the neuron (settable per row)
 *  - When only two types of synapses are used (excitatory and inhibitory),
 *    these can be mapped to the left(right) synaptic input of the neuron.
 *    However, when more than two different synaptic time constants are
 *    desired, these parameters must be mapped to different synaptic input
 *    circuits on different denmems within a compound neurons, which
 *    complicates the routing.
 *
 * This class computes the number of synapse drivers required to realize all
 * synapses, considering all the constraints listed above.
 * As a simplification, we consider only the STP mode, and not the settings for
 * the usable efficacy. The STP time constant must be chosen per HICANN
 * quadrant anyhow.
 *
 * Possible extensions:
 *  - consider the synaptic weight range per row is limited to 16 different
 *    values, typically spanning 1 order of magnitude. We could extract for
 *    each synapse a weight order and request different synapse rows for
 *    different orders of magnitude.
 *  - consider the routing priority
 */
class SynapseDriverRequirements
{
public:
	typedef std::unordered_map<HMF::Coordinate::NeuronOnHICANN, size_t> NeuronWidth;
	typedef std::map<SynapseType, std::map<Side_Parity, size_t> >
		Side_Parity_count_per_synapse_type;
	typedef std::map<Side_Parity, std::vector<HMF::Coordinate::SynapseColumnOnHICANN> >
		SynapseColumnsMap;

	/// constructor
	/// calculates properties of the used compound neurons on the HICANN, like
	/// the number of synapses that can be implemented per synaptic input
	/// granularity.
	///
	/// @param hicann HICANN coordinate
	/// @param nrnpl mapping of biological neurons/populations onto hardware
	/// neurons
	/// @param syn_tgt_mapping  mapping of biological synapse types onto the
	/// synaptic input circuits of the hardware neurons
	SynapseDriverRequirements(
		HMF::Coordinate::HICANNGlobal const& hicann,
		marocco::placement::NeuronPlacementResult const& nrnpl,
		SynapseTargetMapping const& syn_tgt_mapping);

	/// calculate the number of required synapse drivers for the connections
	/// from a L1 route.
	///
	/// Extracts the synaptic connections targetting neurons on the current
	/// HICANN from the hardware projections mapped onto the L1 Route.
	/// For each target neuron, the number synapses per bio synapse property is
	/// counted, cf. class SynapseCounts.
	/// Next the number of required synapse drivers to realize all synapses
	/// considering the hardware configurability constraints is calculated.
	/// In doing so, the number of half synapse rows per hardware synapse
	/// properties and the number of synapses represented by each of the
	/// properties is extracted to fill the two histograms synapse_histogram
	/// and synrow_histogram.
	/// See _calc() for more details.
	///
	/// @param[in] projections list of hardware projections mapped onto the L1
	/// route.
	/// @param[in] graph the PyNN graph of populations and projections
	/// @param[out] synapse_histogram synapses per hardware synapse property
	/// @param[out] synrow_histogram required half synapse rows per hardware
	/// synapse property
	//
	/// @return a std::pair (number of required drivers, number of synapses
	/// from this L1 Route to target neurons on the HICANN)
	std::pair<size_t, size_t> calc(
		std::vector<HardwareProjection> const& projections,
		graph_t const& graph,
		std::map<Side_Parity_Decoder_STP, size_t>& synapse_histogram,
		std::map<Side_Parity_Decoder_STP, size_t>& synrow_histogram) const;

	/// calculate the number of required synapse drivers for the connections
	/// from a L1 route.
	///
	/// @param projections list of hardware projections mapped onto the L1
	/// route.
	/// @param graph the PyNN graph of populations and projections
	///
	/// @return a std::pair (number of required drivers, number of synapses)
	///
	/// @note: this version is used by the algorithm `WaferRouting` to
	/// determine whether any L1Route has targets on a given HICANN. This
	/// assertion could be done much more efficiently, cf. #1594.
	std::pair<size_t, size_t>
	calc(std::vector<HardwareProjection> const& projections, graph_t const& graph) const;

	HMF::Coordinate::HICANNGlobal const& hicann() const {
		return mHICANN;
	}

	std::unordered_map<HMF::Coordinate::NeuronOnHICANN, std::map<SynapseType, SynapseColumnsMap> >
	get_synapse_type_to_synapse_columns_map() const;

private:
	// methods
	// We use static functions to allow an easy testing.

	/// extract the neuron width for all neurons mapped to the used
	/// @param neuron_block_mapping bio neurons mapped onto blocks of hardware
	/// neurons of a HICANN
	static NeuronWidth
	extract_neuron_width(marocco::placement::NeuronBlockMapping const& neuron_block_mapping);

	/// calculate for all used compound hardware neurons the number of target
	/// synapses per synaptic input granularity per biological synapse type.
	/// @param neuron_width neuron width per used hardware neuron
	/// @param syn_tgt_mapping mapping of biological synapse types onto the
	/// synaptic input circuits of the hardware neurons
	static std::unordered_map<HMF::Coordinate::NeuronOnHICANN, Side_Parity_count_per_synapse_type>
	calc_target_synapses_per_synaptic_input_granularity(
		NeuronWidth const& neuron_width, SynapseTargetMapping const& syn_tgt_mapping);

	/// calculate for all used compound hardware neurons the connected synapse
	/// columns per synaptic input granularity per biological synapse type.
	///
	/// @note: this information is not used to calculate the driver
	/// requirements itself, but is used later in the SynapseManager.
	///
	/// @param neuron_width neuron width per used hardware neuron
	/// @param syn_tgt_mapping mapping of biological synapse types onto the
	/// synaptic input circuits of the hardware neurons
	static std::unordered_map<HMF::Coordinate::NeuronOnHICANN,
							  std::map<SynapseType, SynapseColumnsMap> >
	calc_synapse_type_to_synapse_columns_map(
		NeuronWidth const& neuron_width, SynapseTargetMapping const& syn_tgt_mapping);

	/// counts for each synaptic input granularity (column parity, input side)
	/// the number of half synapse rows that are required to realize / the
	/// counts of bio synapse properties passed in `synapses_per_property`.
	///
	/// @param synapses_property synapses per bio property
	/// @param target_synapses_per_parity_and_synaptic_input number of synapses
	/// that can be realized per synaptic input granularity per bio synapse
	/// type.
	///
	/// @note that the total number of required half synapse rows might be
	/// lower than the sum of the entries in the return map. Because certain
	/// bio-properties can be realized with different
	/// col_parity,synaptic_input) pairs.
	static std::map<Type_Decoder_STP, std::map<Side_Parity, size_t> >
	count_half_rows_per_input_granularity(
		std::map<Type_Decoder_STP, size_t> const& synapses_per_property,
		std::map<SynapseType, std::map<Side_Parity, size_t> > const&
			target_synapses_per_parity_and_synaptic_input);


	/// returns the half row count and associated TriParity that most
	/// efficienctly realizes synapses on a given side. This is the TriParity,
	/// which requires the least half synapse rows.
	/// @param side the synaptic input side for which the and synapse column
	/// parity, the number of synapses
	/// @param map a map holding for each combination of synaptic input side
	/// and synapse column parity the number of synapses that can be realized
	/// with one half synapse row.
	///
	/// @return the count and associcated TriParity
	/// @note if the returned count is 0, the return tri parity is undefined.
	static std::pair<size_t, TriParity> get_min_realizing_count_and_parity(
		HMF::Coordinate::Side const& side, std::map<Side_Parity, size_t> const& map);

	/// Counts the number of required half rows required per hardware property
	/// to realize all synapses for one target neuron.  In this step, we choose
	/// the (Side,TriParity) combination to be used for each bio property.  Bio
	/// synapses may be realized on different input granularities with possibly
	/// differing efficiency (in terms of number of half rows required to
	/// realize all synapses). Here, for each bio property, we choose the
	/// (Side,TriParity) combination that requires the least number of half
	/// rows to realize all synapses. The TriParity::any refers to synapses
	/// that can be realized in either column.  This degree of freedom can be
	/// resolved in a future step, and can be used for an efficient use of
	/// resources.
	///
	/// For each bio property, this works as follows:
	/// 1.) For both sides, count the number of half rows needed and an
	///  associated TriParity which most efficiently realizes the requested
	///  synapses.  For this, the function get_min_realizing_count_and_parity()
	///  is used.
	/// 2.) If synapses can be realized on both sides: choose the side requring
	/// less half rows or, if both sides require equal half rows, try to use a
	/// solution with TriParity::any (as it leaves more degree of freedom).  If
	/// both or none sides have TriParity::any, use the left side!
	/// If synapses can be realized only on 1 side, just use it!
	/// 3.) store the chosen combination and the number of required half rows
	/// in the return value.
	/// 4.) store the chosen (Side,TriParity) combination for each bio
	/// property. (Is needed later)
	///
	/// @param[in] half_rows_per_input_granularity required half rows per input
	/// granularity to realize all synapses of a given bio property.
	/// @param[out] bio_to_hw_assignment assignment of bio properties to
	/// intermediate HW properties (Side,TriParity).
	///
	/// @return the number of half rows per (Side,Decoder,STP,TriParity),
	/// initially sorted by TriParity.
	static std::map<TriParity, std::map<Side_Decoder_STP, size_t> > count_half_rows(
		std::map<Type_Decoder_STP, std::map<Side_Parity, size_t> > const&
			half_rows_per_input_granularity,
		std::map<Type_Decoder_STP, Side_TriParity>& bio_to_hw_assignment);

	/// calculate the number of half rows required to realize all synapses to
	/// all target neurons, given the per-target-neuron requirements.
	/// computes for all elements in `required_half_rows_per_neuron` the max
	/// for each present combination of TriParity and Side_Decoder_STP.
	static std::map<TriParity, std::map<Side_Decoder_STP, size_t> > count_half_rows_vec(
		std::vector<std::map<TriParity, std::map<Side_Decoder_STP, size_t> > > const&
			required_half_rows_per_neuron);

	/// count the number of required half rows per TriParity, Side and STP.
	/// I.e., sums over all available Decoder values, per (TriParity,Side,STP).
	/// @param required_half_rows required half rows per
	/// (TriParity,Side,Decoder,STP)
	static std::map<TriParity, std::map<Side_STP, size_t> > count_half_rows_per_side(
		std::map<TriParity, std::map<Side_Decoder_STP, size_t> > const& required_half_rows);

	/// counts the number of synapse rows required per input target side and
	/// STP, in order to realize the half row numbers specified in
	/// `required_half_rows_per_side`.
	///
	/// This function resolves the TriParity degree of freedom.
	/// I.e. requirements, that can be realized with same efficiency in the odd
	/// and even columns (specified by property TriParity::any), are now
	/// distributed to even and odd columns. The actual assignment is returned
	/// by filling the reference argument `half_rows_assigned_to_parity`.
	///
	/// For each Side_STP, this is done as follows:
	/// the load of odd and even columns is tried to be balanced.
	/// In case that
	///  1) there are more half rows required of type "any" than available by
	///  the difference of pure even and odd counts,
	///  2) the remaining "any" half rows are an odd number, then the even
	///  count is 1 higher than the odd one.
	//
	/// @param[in] required_half_rows_per_side required half rows per
	/// (TriParity, Side and STP)
	/// @param[out] half_rows_assigned_to_parity number of half rows assigned
	/// per column parity per (Side,STP) combination. This is currently not
	/// used in the following steps and might be removed.
	static std::map<Side_STP, size_t> count_rows_per_side(
		std::map<TriParity, std::map<Side_STP, size_t> > const& required_half_rows_per_side,
		std::map<Side_STP, std::map<Parity, size_t> >& half_rows_assigned_to_parity);

	/// counts the number of synapse rows required per STP setting.
	/// marginalizes the counts of the different sides.
	static std::map<STPMode, size_t>
	count_rows_per_STP(std::map<Side_STP, size_t> const& required_rows_per_side);

	/// counts the number of synapse drivers required per STP setting.
	/// n_drivers = ceil( n_rows/2.)
	static std::map<STPMode, size_t>
	count_drivers_per_STP(std::map<STPMode, size_t> const& required_rows_per_stp);

	/// counts the number of drivers required to realize all synapses.
	/// computes sum of different STP settings.
	static size_t count_drivers(std::map<STPMode, size_t> const& required_drivers_per_stp);


	/// implementation of the calculation of synapse driver requirements.
	///
	/// The number of required drivers is calculated as follows:
	/// 1.) For each target neuron the number of required half rows per
	/// intermediate hardware synapse property is calculated, i.e. per
	/// (TriParity,Side,Decoder,STP) combination. It can happen that some
	/// synapses can be realized with different (Side, Parity) settings, then
	/// the solution requiring the least number of half rows is used, cf.
	/// count_half_rows().
	/// 2.) For each of these properties the global maximum number of required
	/// half rows is extracted from all neurons.
	/// 3.) From the result of 2), we first sum over all 2 MSB Decoders and
	/// calculate the number of half rows per (TriParity,Side,STP) combination.
	/// Next, we calculate the number of rows required per (Side,STP) setting,
	/// making use of the TriParity degree of freedom.
	/// From this, the number required drivers per STP and the total number of
	/// drivers are obtained.
	///
	/// Finally, the synapse and synapse row histogram per HW property are
	/// filled:
	/// By taking the result from 1.), intermediate assignments from bio
	/// synapse types to (Side,TriParity) combinations are turned into
	/// assignments to (Side,Parity) combinations, which can then be used to
	/// count the synapses per hardware property. See
	/// count_synapses_per_hardware_property() for details.
	/// The synapse row histogram is likewise filled by taking the maximum
	/// count over all neurons per hardware property.
	///
	/// @param[in] syn_counts synapses per bio property per target neuron
	/// @param[in] target_synapses_per_parity_and_synaptic_input for each bio
	/// synapse target, the number of synapses that can be realized per input
	/// granularity (Side,Parity)
	/// @param[out] synapse_histogram synapses per hardware synapse property
	/// @param[out] synrow_histogram required half synapse rows per hardware
	//
	/// @note It is not ensured, that the returned requirements represent the
	/// optimum, i.e. the minimum of drivers sufficient to realize all
	/// synapses. This is because minimal requirements with respect to hardware
	/// synapse properties are first computed per target neuron. This mapping
	/// to hw properties is then taken as fixed when calculating the global
	/// requirements, but there might be more efficient global solutions.
	static std::pair<size_t, size_t> _calc(
		SynapseCounts const& syn_counts,
		std::unordered_map<HMF::Coordinate::NeuronOnHICANN,
						   Side_Parity_count_per_synapse_type> const&
			target_synapses_per_parity_and_synaptic_input,
		std::map<Side_Parity_Decoder_STP, size_t>& synapse_histogram,
		std::map<Side_Parity_Decoder_STP, size_t>& synrow_histogram);

	/// Counts the number of synapses per hardware property for one neuron.
	/// Furthermore, the number of required half rows per hardware property is
	/// computed.
	///
	/// This function is used after the global requirements have been
	/// calculated, to fill the synapse and synapse half row histograms in the
	/// calc() method.
	///
	/// First, synapses with a fixed Parity (TriParity equals even or odd) are
	/// assigned to hardware properties, using the bio to hw assignment passed
	/// in bio_to_hw_assignment`.
	/// Second, synapses with TriParity::any are assigned to hardware
	/// properties such that odd and even synapse columns are used as evenly as
	/// possible per (Side,STP) combination.
	/// The usage of half-synapse rows follows the same principles as in
	/// count_half_rows(), i.e. the total number of used half rows is kept at a
	/// minimum.
	/// In the course of this, also the number of synapses realized per
	/// hardware property are counted.
	///
	/// @param[in] bio_property_counts number of afferent synapses per bio
	/// property for the given target neuron
	/// @param[in] assigned_rows_per_side_stp number of synapse rows
	/// assigned(reserved) for each (Side,STPMode) combination. (global)
	/// this is only used to check for correctness. might be removed (TODO)!
	/// @param[in] bio_to_hw_assignment assignment of bio properties to
	/// intermediate HW properties (Side,TriParity). (neuron-wise)
	/// TODO: we might change the type to
	///       map< STP, map< Type_Decoder, Side_TriParity> >
	/// @param[in] target_synapses_per_parity_and_synaptic_input for each bio
	/// synapse target, the number of synapses that can be realized per input
	/// granularity (Side,Parity)
	///
	/// @param[out] half_rows_per_hardware_property required half rows per
	/// hardware property.
	/// @param[out] assigned_side_parity list of (Side,Parity) combinations
	/// assigned to each bio property. Note that each combination can be
	/// several times in the vector. This map is foreseen to be used for the
	/// implementation with the 'synapse gain with n-th driver' method adopted
	/// from the old flow.
	///
	/// @return for each hardware property the number of assigned synapses
	static std::map<Side_Parity_Decoder_STP, size_t> count_synapses_per_hardware_property(
		// in
		std::map<Type_Decoder_STP, size_t> bio_property_counts,
		std::map<Side_STP, size_t> const& assigned_rows_per_side_stp,
		std::map<Type_Decoder_STP, Side_TriParity> const& bio_to_hw_assignment,
		std::map<SynapseType, std::map<Side_Parity, size_t> >
			target_synapses_per_parity_and_synaptic_input,
		// out
		std::map<Side_Parity_Decoder_STP, size_t>& half_rows_per_hardware_property,
		std::map<Type_Decoder_STP, std::vector<Side_Parity> >& assigned_side_parity);


	// members
	/// Coordinate of HICANN chip we are currently working on.
	HMF::Coordinate::HICANNGlobal mHICANN;

	/// @mapping of biological neurons/populations onto hardware neurons
	marocco::placement::NeuronPlacementResult const& mNeuronPlacement;

	/// mapping of biological synapse types onto the synaptic input circuits of
	/// the hardware neurons
	SynapseTargetMapping const& mSynapseTargetMapping;

	/// the neuron width for all used compound neurons, each represented by the
	/// coordinate of the top left neuron.
	NeuronWidth const mNeuronWidth;

	/// for each used compound hardware neurons, the number of target synapses
	/// per synaptic input granularity per biological synapse type.
	std::unordered_map<HMF::Coordinate::NeuronOnHICANN, Side_Parity_count_per_synapse_type> const
		mTargetSynapsesPerSynapticInputGranularity;

	FRIEND_TEST(SynapseDriverRequirements, Base);
	FRIEND_TEST(SynapseDriverRequirements, SeveralTargets);
	FRIEND_TEST(SynapseDriverRequirements, count_half_rows_per_side);
	FRIEND_TEST(SynapseDriverRequirements, count_rows_per_side);
	FRIEND_TEST(SynapseDriverRequirements, count_drivers);
	FRIEND_TEST(SynapseDriverRequirements, TargetSynapsesPerSynapticInputSimple);
	FRIEND_TEST(SynapseDriverRequirements, TargetSynapsesPerSynapticInputAdvanced);
	FRIEND_TEST(SynapseDriverRequirements, _calc);
	FRIEND_TEST(SynapseDriverRequirements, count_synapses_per_hardware_property);
	FRIEND_TEST(SynapseDriverRequirements, Issue2115);
	FRIEND_TEST(SynapseDriverRequirements, Issue2115_minimal);
};

} // namespace routing
} // namespace marocco

BOOST_CLASS_EXPORT_KEY(::marocco::routing::SynapseCounts)
