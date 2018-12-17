#include "marocco/parameter/HICANNParameters.h"

#include "calibtic/HMF/NeuronCalibration.h"
#include "calibtic/HMF/SynapseRowCalibration.h"
#include "calibtic/backend/Backend.h"
#include "hal/Coordinate/iter_all.h"

#include "marocco/Logger.h"
#include "marocco/parameter/CMVisitor.h"
#include "marocco/parameter/NeuronVisitor.h"
#include "marocco/parameter/SpikeInputVisitor.h"
#include "marocco/routing/util.h"

using namespace HMF::Coordinate;

namespace marocco {
namespace parameter {

using namespace euter;

HICANNParameters::HICANNParameters(
    BioGraph const& bio_graph,
    chip_type& chip,
    pymarocco::PyMarocco const& pymarocco,
    placement::results::Placement const& neuron_placement,
    placement::MergerRoutingResult const& merger_routing,
    routing::results::SynapseRouting const& synapse_routing,
    boost::shared_ptr<calibtic::backend::Backend> const& calib_backend,
    double duration)
    : m_bio_graph(bio_graph),
      m_chip(chip),
      m_pymarocco(pymarocco),
      m_neuron_placement(neuron_placement),
      m_merger_routing(merger_routing),
      m_synapse_routing(synapse_routing),
      m_calib_backend(calib_backend),
      m_duration(duration)
{
}

void HICANNParameters::run()
{
	auto const& hicann = m_chip.index();
	bool const local_neurons = !m_neuron_placement.find(hicann).empty();
	bool const local_synapses =
		m_synapse_routing.has(hicann) && !m_synapse_routing[hicann].synapse_switches().empty();
	bool const external_input_or_transit_only = !local_neurons;
	bool const external_input = ([&]() {
		for (auto const& dnc_merger : iter_all<DNCMergerOnHICANN>()) {
			if (!m_neuron_placement.find(DNCMergerOnWafer(dnc_merger, hicann)).empty()) {
				return true;
			}
		}
		return false;
	})();
	bool const transit_only = external_input_or_transit_only && !external_input;

	if (!transit_only) {
		// Switch on one background generator per DNC merger (for locking).

		if(m_merger_routing.find(hicann.toHICANNOnWafer()) == m_merger_routing.end()) {
			MAROCCO_ERROR("No merger routing information available for " << hicann.toHICANNOnWafer());
			throw std::runtime_error("no merger routing information available");
		}

		auto const& merger_routing_result = m_merger_routing.at(hicann);
		if (merger_routing_result.empty()) {
			MAROCCO_WARN("No background generator available on " << hicann.toHICANNOnWafer());
		}

		bool first = true;
		DNCMergerOnHICANN last_merger;
		for (auto const& nb : iter_all<NeuronBlockOnHICANN>()) {
			if (std::find(merger_routing_result.begin(),
			              merger_routing_result.end(),
						  nb) != merger_routing_result.end()) {
				auto const& item = merger_routing_result[nb];
				// Use the background generator of the first neuron block connected to
				// each DNC merger.
				if (first || last_merger != item) {
					auto const bg = BackgroundGeneratorOnHICANN(nb.toEnum().value());
					MAROCCO_TRACE("Enabling " << bg << " for background on " << nb);
					background_generator(bg, m_pymarocco.bkg_gen_isi);
					first = false;
					last_merger = item;
				}
			}
		}
	}

	// load calibration data from DB
	// const auto calib = getCalibrationData();
	// FIXME: get const calibration not possible, because we need to set speedup. see #1542
	auto calib = getCalibrationData(/*fallback_to_defaults=*/external_input_or_transit_only);

	// v reset for all FG blocks in bio mV
	double v_reset = 0;

	if (local_neurons)
	{
		//const auto& neuron_calib = calib->atNeuronCollection();
		// FIXME: get const calibration not possible, because we need to set speedup. see #1542
		auto neuron_calib = calib->atNeuronCollection();
		neuron_calib->setSpeedup(m_pymarocco.experiment.speedup());

		{
			// 3. transform individual analog parameters
			v_reset = neurons(*neuron_calib);

			if (local_synapses) {
				// transform synapses
				auto synapse_row_calib = calib->atSynapseRowCollection();

				// set default synapse calibration if not existing
				if (synapse_row_calib->size() == 0) {
					MAROCCO_WARN(
						"No synapse calibration available on " << hicann
						<< ". The default synapse trafo will be used instead");
					synapse_row_calib->setDefaults();
				}

				synapses(*synapse_row_calib);
			}
		}
	}

	{
		const auto& shared_calib = calib->atBlockCollection();

		// transforms global analog parameters
		// this needs to be done, for any HICANN used in any way. For
		// example, shared parameter also control L1.
		//
		// v_reset is in pynn units, i.e. mV, shared_parameters takes
		// HW units, i.e. Volts, shift_v is also given in Volts

		double const mV_to_V = 1/1000.;

		shared_parameters(*shared_calib,
		                  v_reset * m_pymarocco.param_trafo.alpha_v * mV_to_V +
						  m_pymarocco.param_trafo.shift_v);
	}
}

void HICANNParameters::neuron_config(neuron_calib_type const& /*unused*/)
{

	m_chip.use_big_capacitors(m_pymarocco.param_trafo.use_big_capacitors);

	// use defaults so far
	//auto& config = m_chip.neurons.config;
	//config.bigcap         = 0x0;
	//config.slow_I_radapt  = 0x0;
	//config.fast_I_radapt  = 0x0;
	//config.slow_I_gladapt = 0x0;
	//config.fast_I_gladapt = 0x0;
	//config.slow_I_gl      = 0x0;
	//config.fast_I_gl      = 0x0;
}

double HICANNParameters::neurons(neuron_calib_type const& calib)
{
	// GLOBAL DIGITAL Neuron Paramets
	neuron_config(calib);

	auto const hicann = m_chip.index();
	auto const& graph = m_bio_graph.graph();

	/* SHARED Analog Neuron Parameteres
	 * For each group of neurons that share analog values we have to agree
	 * on common values.  This has to happen prior to the configuration of
	 * individual neuron parameters as these depend on the shared values.
	 *
	 */
	NeuronSharedParameterRequirements shared_parameter_visitor;
	for (auto const& item : m_neuron_placement.find(hicann)) {
		auto const& params = graph[item.population()]->parameters();
		for (NeuronOnHICANN nrn : item.logical_neuron()) {
			visitCellParameterVector(
				params, shared_parameter_visitor, item.neuron_index(), nrn);
		}
	}

	// INDIVIDUAL Neuron Parameters
	TransformNeurons visitor{m_pymarocco.param_trafo.alpha_v, m_pymarocco.param_trafo.shift_v};

	auto const& synaptic_inputs = m_synapse_routing[m_chip.index()].synaptic_inputs();

	MAROCCO_DEBUG("Configuring neuron parameters on " << hicann);
	for (auto const& item : m_neuron_placement.find(hicann)) {
		auto const& pop = *(graph[item.population()]);
		auto const& logical_neuron = item.logical_neuron();

		// Configure ANALOG neuron parameters.
		for (NeuronOnWafer nrn : logical_neuron) {
			MAROCCO_TRACE("configuring analog parameters for " << nrn);

			transform_analog_neuron(
				calib, pop, item.neuron_index(), nrn, synaptic_inputs,
				visitor, m_chip);
		}

		// As all denmems of a logical neuron will be connected,
		// DIGITAL neuron parameters are only configured for the first denmem.

		NeuronOnHICANN const nrn = logical_neuron.front();
		HMF::HICANN::Neuron& neuron = m_chip.neurons[nrn];

		// Set L1 address
		auto const& address = item.address();
		assert(address != boost::none);
		MAROCCO_TRACE(nrn << " has sending address " << address->toL1Address());
		neuron.address(address->toL1Address());
		neuron.activate_firing(true);
		neuron.enable_spl1_output(true);

		// Connect all denmems belonging to this logical neuron.
		assert(logical_neuron.is_rectangular());
		connect_denmems(nrn, logical_neuron.size());
	}

	auto const v_resets = shared_parameter_visitor.get_v_resets();
	auto const mean_v_reset = shared_parameter_visitor.get_mean_v_reset();

	if(v_resets.size() != 1) {
		MAROCCO_WARN("more than one value for V_reset requested on " << m_chip.index());
		MAROCCO_WARN("only the mean v_reset will be used: " << mean_v_reset << " mV");
		for(auto v_reset : v_resets) {
			MAROCCO_DEBUG("individual v_reset values: " << v_reset << " mV");
		}

	}

	return mean_v_reset;
}

void HICANNParameters::connect_denmems(
	NeuronOnHICANN const& topleft_neuron,
	size_t hw_neurons_size)
{
	size_t const xwidth = hw_neurons_size/2;
	size_t const xmin   = topleft_neuron.x();
	m_chip.connect_denmems(X(xmin), X(xmin+xwidth-1));
}

void HICANNParameters::background_generator(BackgroundGeneratorOnHICANN const& bg, uint32_t isi)
{
	// configure ALL BackgroundGenerators for Repeater & SynapseDriver locking.
	// They are NOT use for production neuron stimulation.

	// FIXME: move direct sthal configuration/access to NICE layer

	HMF::HICANN::BkgRegularISI mode(isi);

	HMF::HICANN::BackgroundGenerator cfg;
	cfg.enable(true);
	cfg.seed(0);
	cfg.address(HMF::HICANN::L1Address(0));
	cfg.set_mode(mode);

	m_chip.layer1[bg] = cfg;
}

void HICANNParameters::shared_parameters(
	shared_calib_type const& calib,
	double v_reset)
{
	using namespace HMF;

	for (size_t ii=0; ii<HMF::Coordinate::FGBlockOnHICANN::enum_type::size; ++ii)
	{
		// default values for other parameters are also retrieved
		auto hwparams = calib.applySharedCalibration(v_reset, ii);
		FGBlockOnHICANN fgb{Enum{ii}};
		auto& fg = m_chip.floating_gates;
		hwparams.toHW(fgb, fg);
	}
}

void HICANNParameters::synapses(synapse_row_calib_type const& calib)
{
	HMF::Coordinate::typed_array<double, NeuronOnHICANN> const weight_scales = weight_scale_array();

	auto const& hicann = m_chip.index();
	auto const& synapses = m_synapse_routing.synapses();
	for (auto const row : iter_all<SynapseRowOnHICANN>()) {
		// array storing scaled weights in nanoSiemens, 0. if not used
		std::array<double, NeuronOnHICANN::x_type::size> scaled_weights{{}};
#ifndef MAROCCO_NDEBUG
		// array storing bio weights in micro Siemens, 0. if not used
		std::array<double, NeuronOnHICANN::x_type::size> bio_weights{{}};
#endif // MAROCCO_NDEBUG

		// Collect scaled weights for synapses in current row
		// Note: In the future, multiple hardware synapses per bio synapse might be used
		// to extend the dynamic range.
		for (auto const& item : synapses.find(hicann, row)) {
			auto const synapse = item.hardware_synapse();
			assert(synapse != boost::none);

			auto const edge = m_bio_graph.edge_from_id(item.edge());
			auto const& proj_view = m_bio_graph.graph()[edge];

			size_t const src_neuron_in_proj_view = routing::to_relative_index(
				proj_view.pre().mask(), item.source_neuron().neuron_index());
			size_t const trg_neuron_in_proj_view = routing::to_relative_index(
				proj_view.post().mask(), item.target_neuron().neuron_index());

			double const bio_weight =
				proj_view.getWeights()(src_neuron_in_proj_view, trg_neuron_in_proj_view);

			double const w_scale = weight_scales[synapse->toNeuronOnHICANN()];
			assert(w_scale > 0.); // check for inconsistency between routing and placement

			size_t const col = synapse->x().value();
			// scale and transform
			scaled_weights[col] = bio_weight * w_scale * 1000. /*uS to nS*/;
#ifndef MAROCCO_NDEBUG
			bio_weights[col] = bio_weight;
#endif // MAROCCO_NDEBUG
		}

		// compute max weight and find best gmax configuration
		double const max_weight = *(std::max_element(scaled_weights.cbegin(), scaled_weights.cend()));

		// get copy and not const ref, because findBestGmaxConfig is not const.
		HMF::SynapseRowCalibration row_calib =
		    *boost::dynamic_pointer_cast<HMF::SynapseRowCalibration const>(calib.at(row));

		HMF::GmaxConfig const gc = row_calib.findBestGmaxConfig(max_weight);
		auto const& synapse_trafo = row_calib.at(gc);

		auto row_config_proxy = m_chip.synapses[row]; // proxy object that holds references
		for (size_t col = 0; col < scaled_weights.size(); ++col) {
			if (scaled_weights[col] > 0.) {
				double const scaled_weight = scaled_weights[col];
				HMF::HICANN::SynapseWeight const hw_weight =
				    synapse_trafo->getDigitalWeight(scaled_weight);
				// store weight
				row_config_proxy.weights[col] = hw_weight;

#ifndef MAROCCO_NDEBUG
				SynapseOnHICANN syn_addr(row, SynapseColumnOnHICANN(col));
				double clipped_weight = synapse_trafo->getAnalogWeight(hw_weight);
				clipped_weight = clipped_weight / weight_scales[syn_addr.toNeuronOnHICANN()] /
				                 1000. /*nS to uS*/;
				MAROCCO_TRACE(
					"synapse weight of " << syn_addr << " set to " << hw_weight << ", bio weight "
				    << bio_weights[col] << ", clipped bio weight " << clipped_weight);
#endif // MAROCCO_NDEBUG
			}
		}

		// store gmax config
		auto& driver_config = m_chip.synapses[row.toSynapseDriverOnHICANN()];
		HMF::HICANN::RowConfig& driver_row_config = driver_config[row.toRowOnSynapseDriver()];

		// selects 1 of 4 V_max values from global FGs
		driver_row_config.set_gmax(gc.get_sel_Vgmax());

		// gmax divider [1..15]
		// use same config for left and right synaptic input
		driver_row_config.set_gmax_div(left, gc.get_gmax_div());
		driver_row_config.set_gmax_div(right, gc.get_gmax_div());

		// TODO (BV): store V_gmax, depending on ESS or not ESS
	}
}

HMF::Coordinate::typed_array<double, NeuronOnHICANN> HICANNParameters::weight_scale_array() const
{
	CMVisitor const cm_visitor{};
	HMF::Coordinate::typed_array<double, NeuronOnHICANN> rv;

	//initialize all values to 0.
	for (auto const& noh : iter_all<NeuronOnHICANN>())
		rv[noh] = 0.;

	auto const& use_bigcap = m_chip.neurons.config.bigcap;

	auto const& graph = m_bio_graph.graph();

	// We need to calculate the scaling factor for each logical neuron.
	for (auto const& item : m_neuron_placement.find(m_chip.index())) {
		auto const& params = graph[item.population()]->parameters();
		auto const& logical_neuron = item.logical_neuron();

		// Sum up the capacity of the connected denmems on the hardware.
		double cm_hw = 0.;
		std::vector<NeuronOnHICANN> connected_neurons;
		connected_neurons.reserve(logical_neuron.size());
		for (NeuronOnHICANN nrn : item.logical_neuron()) {
			// We have to consider different capacitor choices on top / bottom neuron blocks.
			cm_hw += use_bigcap[nrn.y()] ? HMF::NeuronCalibration::big_cap
			                             : HMF::NeuronCalibration::small_cap;
			connected_neurons.push_back(nrn);
		}

		double const cm_bio = visitCellParameterVector(params, cm_visitor, item.neuron_index());
		double const scale = m_pymarocco.experiment.speedup() * cm_hw / cm_bio;

		for (auto cnrn : connected_neurons) {
			rv[cnrn] = scale;
		}
	}
	return rv;
}

boost::shared_ptr<HICANNParameters::calib_type>
HICANNParameters::getCalibrationData(bool fallback_to_defaults)
{
	using pymarocco::PyMarocco;

	auto calib = boost::make_shared<calib_type>();
	if (!m_calib_backend) {
		calib->setDefaults();
	} else {
		calibtic::MetaData md;

		const size_t hicann_id = m_chip.index().toHICANNOnWafer().id().value();
		std::stringstream calib_file;
		calib_file << "w" << size_t(m_chip.index().toWafer()) << "-h";
		calib_file << hicann_id;
		const std::string calib_file_string = calib_file.str();

		MAROCCO_TRACE("loading calibration for " << calib_file_string << " from "
		                                         << m_pymarocco.calib_path);
		try {
			m_calib_backend->load(calib_file_string, md, *calib);
		} catch (std::runtime_error const& err) {
			if (!fallback_to_defaults) {
				throw;
			}
			MAROCCO_WARN(err.what());
			MAROCCO_WARN("Will use default calibtration");
			calib->setDefaults();
		}
	}

	if(calib->getPLLFrequency() != m_pymarocco.pll_freq) {
		MAROCCO_WARN("PLL stored in HICANNCollection "
		             << int(calib->getPLLFrequency()/1e6) << " MHz != "
		             << int(m_pymarocco.pll_freq/1e6) << " MHz set here.");
	}

	return calib;
}

} // namespace parameter
} // namespace marocco
