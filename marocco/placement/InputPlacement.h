#pragma once

#include <array>
#include <memory>

#include "marocco/assignment/PopulationSlice.h"
#include "marocco/config.h"
#include "marocco/graph.h"
#include "marocco/placement/MergerRoutingResult.h"
#include "marocco/placement/internal/Result.h"
#include "marocco/placement/parameters/InputPlacement.h"
#include "marocco/placement/parameters/L1AddressAssignment.h"
#include "marocco/placement/parameters/ManualPlacement.h"
#include "marocco/placement/parameters/NeuronPlacement.h"
#include "marocco/placement/results/Placement.h"

namespace marocco {
namespace placement {

/**
 * Assign addresses to external spike source populations and map onto
 * output buffers.
 * @pre Neuron placement and merger routing has been completed,
 *    see \c HICANNPlacement and \c MergerRouting.
 *
 * TODO: add general description of algorithm from SJ's thesis.
 *
 * Bandwidth-Aware Input placement:
 * ===============================:
 *
 * If pymarocco.input_placement.consider_rate is true, the input placement
 * considers the bandwith for spikes in the Layer 2 network, to avoid spike loss
 * in the layer 2 network. There are two bottlenecks: the maximum pulse rate one
 * FPGA can send (`max_rate_FPGA`), and the maximum pulse rate before the HICANN
 * link (`max_rate_HICANN`) saturates.  From the spike sources the expected mean
 * firing rate is extracted, see \c FiringRateVisitor for details.
 * Spike sources are placed in the same order as when not considering the rates. 
 * If the still available bandwidth per HICANN or FPGA is not sufficient for a
 * spike source, the next free input link is checked.
 *
 * By setting the parameter `bandwidth_utilization` to a value below 1, one can
 * account for the fact that the *mean* rate is extracted from the spike
 * sources, but the actual rate can be temporarily higher, e.g. for Poisson
 * spike trains. Eventually, only the fraction `bandwidth_utilization` of the
 * full bandwidth per HICANN or FPGA is used.
 *
 * The implementation is valid for both Layer 2 Architectures:
 * Old: Virtex FPGA + 4 DNC for 4 reticles
 * New: Kintex FPGA for 1 reticle
 */
struct InputPlacement
{
public:
	InputPlacement(
		graph_t const& graph,
		parameters::InputPlacement const& parameters,
		parameters::ManualPlacement const& manual_placement,
		parameters::NeuronPlacement const& neuron_placement_parameters,
		parameters::L1AddressAssignment const& l1_address_assignment,
		MergerRoutingResult const& merger_routing,
		double speedup,
		hardware_system_t& hw,
		resource_manager_t& mgr);

	/**
	 * @param neuronpl Result of neuron placement step.
	 * @param output_mapping Input/output parameter
	 *    that contains the results of the \c MergerRouting step
	 *    and is amended with the results of the input placement.
	 */
	void run(
		results::Placement& neuron_placement,
		internal::Result::address_assignment_type& address_assignment);

private:
	void configureGbitLinks(
		HMF::Coordinate::HICANNGlobal const& hicann,
		internal::L1AddressAssignment& address_assignment);

	/**
	 * @brief Place as many neurons as possible on the DNC mergers of the given HICANN.
	 * @param[in,out] bio Population slice whose neurons should be placed.
	 * @note The caller of this function has to check for remaining neurons in the
	 *       passed-in population slice.
	 */
	void insertInput(
		HMF::Coordinate::HICANNOnWafer const& target_hicann,
		results::Placement& neuron_placement,
		internal::L1AddressAssignment& address_assignment,
		marocco::assignment::PopulationSlice& bio);

	graph_t const&           mGraph;
	parameters::InputPlacement const& m_parameters;
	parameters::ManualPlacement const& m_manual_placement;
	parameters::NeuronPlacement const& m_neuron_placement_parameters;
	parameters::L1AddressAssignment const& m_l1_address_assignment;
	MergerRoutingResult const& m_merger_routing;
	double const m_speedup;
	hardware_system_t&       mHW;
	resource_manager_t&      mMgr;

	////////////////////////////
	// bandwidth aware placement
	////////////////////////////

	typedef double rate_type;

	/// returns the still available rate on a HICANN in Hz.
	/// This considers the still available rate on the associated FPGA as well
	/// as the PyMarocco.input_placement.bandwidth_utilization parameter.
	/// @param hicann coordinate of HICANN
	/// @return the available Rate in Hz
	rate_type availableRate(HMF::Coordinate::HICANNOnWafer const& hicann);

	/// allocates a firing rate as used for a HICANN and the associated FPGA.
	/// @param hicann coordinate of HICANN
	/// @param the rate in Hz to be allocated
	void allocateRate(HMF::Coordinate::HICANNOnWafer const& hicann, rate_type rate);

	/// counts the number of neurons from a population size that do not exceed
	/// the available rate.
	///
	/// @param bio population slice of spike sources
	/// @param max_neurons maximum number of neurons from the back of the
	///        slice that are checked for fitting into the available rate.
	/// @param available_rate availalbe rate (bandwidth) in Hz.
	//
	/// @return pair of number or neurons fitting into available rate as first 
	///         argument, and the total rate used by these neurons as 2nd
	///         argument.
	std::pair< size_t, rate_type >
	neuronsFittingIntoAvailableRate(
			marocco::assignment::PopulationSlice const& bio,
			size_t max_neurons,
			rate_type available_rate
			) const;


	/// already used pulse rate in Hz per HICANN
	std::unordered_map<HMF::Coordinate::HICANNOnWafer, rate_type> mUsedRateHICANN;

	/// already used pulse rate in Hz per FPGA
	std::unordered_map<HMF::Coordinate::FPGAOnWafer, rate_type> mUsedRateFPGA;

	/// maximum pulse rate per HICANN in Hz (17.8 MHz)
	/// assumed limitation: 1 pulse per 56 ns for slow LVDS mode
	static const rate_type max_rate_HICANN;

	/// maximum pulse rate per FPGA in Hz (125 MHz)
	/// assumed limitation: 1 pulse per FPGA clock cycle of 8ns
	static const rate_type max_rate_FPGA;

};

} // namespace placement
} // namespace marocco
