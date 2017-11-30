#pragma once

#include <iosfwd>
#include <list>
#include <map>
#include <numeric>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "marocco/routing/SynapseDriverRequirements.h"
#include "marocco/routing/results/ConnectedSynapseDrivers.h"

namespace marocco {
namespace routing {


/** The synapse manager is responsible for distributing the half synapse rows
 * of the assigned synapse drivers to the respective hardware synapse
 * properties: side of synaptic input, parity of synapse column, driver decoder
 * address, and STP setting
 *
 * The SynapseManager is constructed with the result of the Driver Assignment
 * algorithm (Fieres).  Then, the information about originally required half synrows and
 * synapses hw property is added with the #init() method, which also distributes and
 * allocates available resources to the different hardware synapse properties.
 */
class SynapseManager
{
public:
	typedef std::unordered_map<HMF::Coordinate::VLineOnHICANN,
	                           std::vector<results::ConnectedSynapseDrivers> >
		Result;
	typedef std::vector<HMF::Coordinate::SynapseRowOnHICANN> SubRows; // vector of synapse row
	                                                                  // coordinates, used to map
	                                                                  // half synapse rows HW
	                                                                  // synapse properties
	typedef std::map<Side_Parity_Decoder_STP, SubRows> Assignment; // assigned half synapse rows for
																   // each HW synapse property

	typedef std::map<Side_Parity_Decoder_STP, size_t> Histogram;
	typedef std::unordered_map<HMF::Coordinate::VLineOnHICANN, Histogram> HistMap;

	/// constructs the synapse row manager with results from driver assignment
	/// algorithm
	SynapseManager(Result const& resources);

	/// for each input vline, distribute the half synapse rows of the assigned
	/// drivers to the different HW synapse properties.
	/// If less resources are assigned than needed, the same relative assignment
	/// process as in the Fieres Algorithm for Synapse drivers is done.
	/// This relative reduction is done first per STP setting (drivers), then
	/// per input side(rows), and finally per 2MSB pattern (half rows).
	/// Always the resources are distributed relative to the number of synapses
	/// they represent.
	/// @note this procedure may lead to bad results for inhomogeneous
	/// connectivity. See the note in function `relative_reduction`.
	///
	/// @param synapse_hist number of synapses represented per hardware synapse
	/// property per incoming connection.
	/// @param synapse_hist number of required half synapse rows per hardware
	/// synapse property per incoming connection.
	void init(HistMap const& synapse_hist, HistMap const& synrow_hist);

	/// Returns the list of half synapse rows assigned to a vline for a given
	/// HW Synapse Property
	SubRows const& getRows(
		HMF::Coordinate::VLineOnHICANN const& vline,
		Side_Parity_Decoder_STP const& hw_synapse_property);

	/// get the assignment for on vline.
	/// this is used to get the STP mode, the Side and Decoder settings to
	/// configure the synapse drivers.
	Assignment const& get(HMF::Coordinate::VLineOnHICANN const& vline) const;

	/// checks that for all incoming routes (vlines), that the number of
	/// concatenated synapses drivers does not exceed the maximumm allowed
	/// chain length.
	/// @param chain_length maximum allowed chain length
	/// @note currently does not check anything (FIXME). This method was
	/// literally copied from the class SynapseRowManager.
	void check(size_t chain_length);

	// pre-decl
	struct SynapsesOnVLine;
	typedef std::map<Side_Parity, std::vector<HMF::Coordinate::SynapseColumnOnHICANN> >
		SynapseColumnsMap;

	/// get the manager for the synapses for one vline (incoming route).
	/// @param vline VLine coordinate of the incoming route.
	/// @param synapse_columns for each neuron a map from bio synapse type to
	/// synapse columns and input sides.
	/// @note can be called only once per vline.
	SynapsesOnVLine getSynapses(
		HMF::Coordinate::VLineOnHICANN const& vline,
		std::unordered_map<HMF::Coordinate::NeuronOnHICANN,
						   std::map<SynapseType, SynapseColumnsMap> > const& synapse_columns);


	/// helper class to step through synapses implementing the same bio synapse
	/// property for one vline and one target neuron.
	///
	/// Each vline has been assigned certain half rows for the different hw
	/// synapse properties. A given bio synapse property must match the STP and
	/// the decoder setting, while there can be various input sides and columns
	/// implementing the respective bio synapse type. The SynapseStepper steps
	/// through all available synapses fulfilling this requirement.
	///
	/// usage:
	///  if (stepper.has_synapses())
	///    auto synapse_coord = stepper.get();
	struct SynapseStepper
	{
	public:
		/// constructor.
		/// @param assignment assigment of hardware synapse properties to half
		/// synapse rows
		/// @param dec_stp the requested driver decoder and the STP setting of
		/// the bio synapse property
		/// @param s_p_to_c_map for each input granularity (Side,Parity), a
		/// list of synapse columns, that are suitable for implementing synapses
		/// with the required synapse type. The Parity of the synapse columns
		/// must match the parity of the associated key.
		SynapseStepper(
			Assignment const& assignment,
			Decoder_STP dec_stp,
			SynapseColumnsMap const& s_p_to_c_map);

		/// returns true, if there are still synapses available.
		bool has_synapses() const
		{
			return _has_synapses;
		}

		/// returns a suitable synapse coordinate.
		/// @note each synapse is returned only once, i.e. a call to get()
		/// advances the internal cursors to point to the next free synapse.
		/// @throws std::runtime_error if no synapse is available.
		HMF::Coordinate::SynapseOnHICANN get();

	private:
		/// advances the iterators to point to the next free synapse.
		/// If there is no synapse left, member `_has_synapses` is set to false.
		void update();

		/// assigment of hardware synapse properties to half synapse rows
		Assignment const& hw_prop_to_half_row;

		/// requested driver decoder and STP setting of the bio synapse property
		Decoder_STP const decoder_stp;

		/// for each input granularity (Side,Parity), a list of synapse
		/// columns, that are suitable for implementing synapses with the
		/// required synapse type.
		SynapseColumnsMap const& side_parity_to_columns_map;

		/// iterator pointing to the column of the next free synapse.
		std::vector<HMF::Coordinate::SynapseColumnOnHICANN>::const_iterator syn_col_it;

		/// iterator pointing to the currently active (Side,Parity) combination
		SynapseColumnsMap::const_iterator side_parity_it;

		/// iterator pointing to current item in hw property to subrow map
		Assignment::const_iterator hw_prop_to_half_row_it;

		/// iterator pointing to the row of the next free synapse
		SubRows::const_iterator sub_row_it;

		/// stores, whether there are still synapses available.
		bool _has_synapses;
	};


	/// manages all synapses available for a VLine (an incoming route).
	/// stores for each combination of bio synapse property and target neuron a
	/// SynapseStepper, which takes care of available synapses for the given
	/// combination. The SynapseSteppers are created, when first requested.
	struct SynapsesOnVLine
	{
	public:
		typedef std::map<SynapseType, SynapseColumnsMap> TypeToSynapsesColumnsMap;

		/// constructor
		/// @param TypeToSynapseColumnsMapPerNeuron for each neuron a map from
		/// bio synapse type to synapse columns and input sides.
		/// @param assignment assigment of hardware synapse properties to half
		/// synapse rows.
		SynapsesOnVLine(
			std::unordered_map<HMF::Coordinate::NeuronOnHICANN, TypeToSynapsesColumnsMap> const&
				TypeToSynapseColumnsMapPerNeuron,
			Assignment const& assignment);

		/// get a synapse to a desired target with desired bio synapse property.
		///
		/// @param target_nrn coordinate of the top left neuron of compound neuron.
		/// @param bio_synapse_property bio synapse property
		///
		/// @returns a pair, with a synapse coordinate as first element, and a
		/// bool as second element. If the bool is set to true, the synapse
		/// coordinate in the first element is valid.  If the bool is set to
		/// false, no suitable synapse is available, and the returned synapse
		/// coordinate must not be used.
		std::pair<HMF::Coordinate::SynapseOnHICANN, bool> getSynapse(
			HMF::Coordinate::NeuronOnHICANN const& target_nrn,
			Type_Decoder_STP const& bio_synapse_property);

	private:
		/// for each neuron a map from bio synapse type to synapse columns and
		/// input sides.
		std::unordered_map<HMF::Coordinate::NeuronOnHICANN, TypeToSynapsesColumnsMap> const&
			mTypeToSynapseColumnsMapPerNeuron;

		/// assigment of hardware synapse properties to half synapse rows
		Assignment const& mAssignedHalfRows;

		/// for each target neuron, a map from bio synapse property to a
		/// SynapseStepper managing all suitable synapses.
		std::unordered_map<HMF::Coordinate::NeuronOnHICANN,
						   std::map<Type_Decoder_STP, SynapseStepper> > mBioSynapsesPerNeuron;
	};


	friend std::ostream& operator<<(std::ostream& os, SynapseManager const& mgr);

	/// distributes resources to different properties relative to the priority
	/// of properties.
	///
	/// If the sum of requested resources in `required` is larger than the
	/// number of available resources, the resources are distributed relative to
	/// their priority.
	/// Shortcuts:
	///  A = available resources,
	///  P = priority of a resource,
	///  Ptot = sum of all prioriets.
	/// First, properties get assigned D = max(1, floor(A*P/Ptot)) resources
	/// If the sum of all D is higher or lower than A, then the following
	/// procedure applies:
	/// For each property, the delta = D - A*P/Ptot is calculated.
	/// If Sum(D) > A, then D of the property with the biggest delta is reduced
	/// by 1. This is repeated until Sum(D) = A.
	/// If Sum(D) < A, then D of the property with the lowest delta is
	/// increased by 1.  This is repeated until Sum(D) = A.
	///
	/// @note the number of required resources per property is ignored, if the
	/// total number of requested resources is higher than available. Hence,
	/// some properties might end up with more resources than required, while
	/// other receive less than requested.
	///
	/// @tparam Key the property - any type that usable as key for a std::map
	///
	/// @param required number of required resources per property
	/// @param priority the priority per property (the number of synapses
	/// realized per property)
	/// @param available total number of available resources
	///
	/// @return a std::map with distributed resources per property
	template <typename Key>
	static std::map<Key, size_t> relative_reduction(
		std::map<Key, size_t> const& required,
		std::map<Key, size_t> const& priority /*synapses*/,
		size_t const available);

private:
	/// tracks the current synapse row assignment for each local route and each
	/// combination of MSBs and exc/inh. For all this combinations we need /
	/// individual SynapseRows, because the necessary configuration is part of /
	/// that row and not the individual synapse.
	std::unordered_map<HMF::Coordinate::VLineOnHICANN /*route id*/, Assignment> mAllocation;

	/// mapping of vlines to assigned Synapse Drivers
	std::unordered_map<HMF::Coordinate::VLineOnHICANN,
	                   std::list<HMF::Coordinate::SynapseDriverOnHICANN> >
		mDrivers;

	/// the number of drivers assigned to each vline
	std::unordered_map<HMF::Coordinate::VLineOnHICANN, size_t> mLines;

	/// holds all vlines, for which a SynapsesOnVLine instance was created
	/// helps to ensure that getSynapses(..) is only called once per vline.
	std::unordered_set<HMF::Coordinate::VLineOnHICANN> mSynapsesOnVLineCreated;

	FRIEND_TEST(SynapseManager, SynapseStepper);
};

std::ostream& operator<<(std::ostream& os, SynapseManager const& mgr);

// helpers

template <typename Key, typename T>
struct map_acc
{
	T operator()(T const& lhs, std::pair<Key, T> const& rhs)
	{
		return lhs + rhs.second;
	}
};

struct Most
{
	template <typename T>
	bool operator()(T const& a, T const& b)
	{
		return a > b;
	}
};

// implementation
template <typename Key>
std::map<Key, size_t> SynapseManager::relative_reduction(
	std::map<Key, size_t> const& required,
	std::map<Key, size_t> const& priority /*synapses*/,
	size_t const available)
{
	// todo: check equality of keys;
	size_t const required_total =
		std::accumulate(required.begin(), required.end(), 0, map_acc<Key, size_t>());
	size_t const priority_total =
		std::accumulate(priority.begin(), priority.end(), 0, map_acc<Key, size_t>());

	std::map<Key, size_t> assigns;
	size_t assigned = 0;

	if (required_total <= available) {
		assigns = required;
		assigned = required_total;
	} else {
		// we didn't get sufficient drivers, now assign relative to number of synapses
		typedef std::multimap<double, Key, Most> Map;
		Map too_many;
		Map too_few;

		for (auto const& item : required) {
			auto const& key = item.first;
			if (item.second == 0) {
				assigns[key] = 0;
				continue;
			}
			double const rel = double(priority.at(key)) / priority_total * double(available);
			size_t const t = std::max<size_t>(1, rel);
			assigns[key] = t;
			assigned += t;

			double const delta = double(t) - rel;
			if (delta > 0.) {
				// too many
				too_many.insert(std::make_pair(delta, key));
			} else {
				// too few
				too_few.insert(std::make_pair(std::abs(delta), key));
			}
		}

		// too many assigned
		while (assigned > available) {
			for (auto it = too_many.begin(); it != too_many.end(); ++it) {
				size_t& val = assigns[it->second];
				if (val > 0) {
					assigned--;
					val--;
				}

				if (assigned == available) {
					break;
				}
			}
		}

		// too few assigned
		while (assigned < available) {
			for (auto it = too_few.begin(); it != too_few.end(); ++it) {
				size_t& val = assigns[it->second];
				val++;
				assigned++;

				if (assigned == available) {
					break;
				}
			}
		}
	}
	return assigns;
}


} // namespace routing
} // namespace marocco
