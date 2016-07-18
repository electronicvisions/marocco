#pragma once

#include <unordered_map>

#include "HMF/BlockCollection.h"
#include "HMF/HICANNCollection.h"
#include "HMF/NeuronCollection.h"
#include "sthal/HICANN.h"
#include "hal/Coordinate/typed_array.h"

#include "marocco/BioGraph.h"
#include "marocco/placement/Result.h"
#include "marocco/routing/Result.h"
#include "pymarocco/PyMarocco.h"

namespace marocco {
namespace parameter {

class HICANNParameters
{
public:
	typedef sthal::HICANN chip_type;
	typedef HMF::HICANNCollection calib_type;
	typedef HMF::NeuronCollection neuron_calib_type;
	typedef HMF::BlockCollection shared_calib_type;
	typedef HMF::SynapseRowCollection synapse_row_calib_type;

	/**
	 * @param duration PyNN experiment duration in ms
	 */
	HICANNParameters(
		BioGraph const& bio_graph,
		chip_type& chip,
		pymarocco::PyMarocco const& pymarocco,
		placement::Result const& placement,
		routing::Result const& routing,
		double duration);
	~HICANNParameters();

	void run();

private:
	// returns mean v_reset in bio mV
	double neurons(
		neuron_calib_type const& calib,
		placement::results::Placement const& neuron_placement,
		routing::SynapseTargetMapping const& synapse_target_mapping);

	void connect_denmems(
		HMF::Coordinate::NeuronOnHICANN const& topleft_neuron,
		size_t hw_neurons_size);

	void neuron_config(neuron_calib_type const& calib);

	void analog_output(
	    neuron_calib_type const& calib,
	    placement::results::Placement const& neuron_placement);

	void spike_input(
		placement::results::Placement const& neuron_placement);

	void background_generators(uint32_t isi=500);

	/**
	 * @param v_reset Reset voltage in hardware V
	 */
	void shared_parameters(shared_calib_type const& calib, double v_reset);

	void synapses(
		synapse_row_calib_type const& calib,
		routing::synapse_driver_mapping_t::result_type const& synapse_routing,
		placement::results::Placement const& neuron_placement);

	/// returns an array with the weight scale factor for each neuron on the hicann.
	/// The factor to scale biological to hardware weights is calculated as: speedup * cm_hw/ cm_bio
	/// where cm_hw is the sum of the capacitances of all interconnected hw-neurons
	HMF::Coordinate::typed_array<double, HMF::Coordinate::NeuronOnHICANN> weight_scale_array(
		placement::results::Placement const& neuron_placement) const;

	boost::shared_ptr<calib_type> getCalibrationData();

	boost::shared_ptr<calibtic::backend::Backend>
	getCalibticBackend();

	BioGraph const& m_bio_graph;
	chip_type& m_chip;
	pymarocco::PyMarocco const& m_pymarocco;
	placement::Result const& m_placement;
	routing::Result const& m_routing;
	double m_duration;

	HMF::Coordinate::typed_array<std::vector<sthal::Spike>, HMF::Coordinate::DNCMergerOnHICANN> m_spikes;
};

} // namespace parameter
} // namespace marocco
