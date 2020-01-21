#pragma once

#include <unordered_map>

#include "calibtic/HMF/BlockCollection.h"
#include "calibtic/HMF/HICANNCollection.h"
#include "calibtic/HMF/NeuronCollection.h"
#include "sthal/HICANN.h"
#include "halco/common/typed_array.h"

#include "marocco/BioGraph.h"
#include "marocco/placement/results/Placement.h"
#include "marocco/placement/Result.h"
#include "marocco/routing/results/SynapseRouting.h"
#include "pymarocco/PyMarocco.h"

namespace calibtic {
namespace backend {

class Backend;

} // namespace backend
} // namespace calibtic

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
		placement::results::Placement const& neuron_placement,
		placement::MergerRoutingResult const& merger_routing,
		routing::results::SynapseRouting const& synapse_routing,
		boost::shared_ptr<calibtic::backend::Backend> const& calib_backend,
		double duration);

	void run();

private:
	// returns mean v_reset in bio mV
	double neurons(neuron_calib_type const& calib);

	void connect_denmems(
		halco::hicann::v2::NeuronOnHICANN const& topleft_neuron,
		size_t hw_neurons_size);

	void neuron_config(neuron_calib_type const& calib);

	void background_generator(halco::hicann::v2::BackgroundGeneratorOnHICANN const& bg,
	                          uint32_t isi = 500);

	/**
	 * @param v_reset Reset voltage in hardware V
	 */
	void shared_parameters(shared_calib_type const& calib, double v_reset);

	void synapses(
		synapse_row_calib_type const& calib);

	/// returns an array with the weight scale factor for each neuron on the hicann.
	/// The factor to scale biological to hardware weights is calculated as: speedup * cm_hw/ cm_bio
	/// where cm_hw is the sum of the capacitances of all interconnected hw-neurons
	halco::common::typed_array<double, halco::hicann::v2::NeuronOnHICANN> weight_scale_array() const;

	boost::shared_ptr<calib_type> getCalibrationData(bool fallback_to_defaults);

	BioGraph const& m_bio_graph;
	chip_type& m_chip;
	pymarocco::PyMarocco const& m_pymarocco;
	placement::results::Placement const& m_neuron_placement;
	marocco::placement::MergerRoutingResult const& m_merger_routing;
	routing::results::SynapseRouting const& m_synapse_routing;
	boost::shared_ptr<calibtic::backend::Backend> m_calib_backend;
	double m_duration;

	halco::common::typed_array<std::vector<sthal::Spike>, halco::hicann::v2::DNCMergerOnHICANN> m_spikes;
};

} // namespace parameter
} // namespace marocco
