#include "marocco/parameter/HICANNParameter.h"

#include <tbb/parallel_for_each.h>
#include <algorithm>
#include <memory>
#include <chrono>
#include <string>
#include <sstream>
#include <cstdlib>

#include "calibtic/backend/Library.h"
#include "HMF/NeuronCalibration.h"
#include "HMF/SynapseRowCalibration.h"

#include "sthal/FGStimulus.h"
#include "hal/Coordinate/iter_all.h"

#include "marocco/Logger.h"
#include "marocco/config.h"
#include "marocco/parameter/AnalogVisitor.h"
#include "marocco/parameter/NeuronVisitor.h"
#include "marocco/parameter/Result.h"
#include "marocco/parameter/SpikeInputVisitor.h"
#include "marocco/parameter/CMVisitor.h"
#include "marocco/util.h"
#include "marocco/util/chunked.h"

#include "marocco/routing/util.h"


using namespace HMF::Coordinate;
using namespace HMF;

namespace marocco {
namespace parameter {

std::unique_ptr<HICANNParameter::result_type>
HICANNParameter::run(
	result_type const& _placement,
	result_type const& _routing)
{
	auto const& placement = result_cast<placement::Result>(_placement);
	auto const& routing   = result_cast<routing::Result>(_routing);

	std::unordered_map<HICANNOnWafer, HICANNTransformator::CurrentSources> current_source_map;

	auto const& neuron_placement = placement.neuron_placement;
	for (auto const& entry : mCurrentSourceMap)
	{
		Vertex const v = entry.first;
		size_t const nrn = entry.second.first;

		// There should be exactly one result for this lookup.
		auto iterable = neuron_placement.find(BioNeuron(v, nrn));
		assert(iterable.begin() != iterable.end());
		assert(std::next(iterable.begin()) == iterable.end());

		auto const& logical_neuron = iterable.begin()->logical_neuron();
		assert(!logical_neuron.is_external());

		auto const primary_neuron = logical_neuron.front();

		auto ptr = boost::dynamic_pointer_cast<StepCurrentSource const>(entry.second.second);
		if (!ptr) {
			MAROCCO_WARN("unsupported current type");
		} else {
			MAROCCO_DEBUG("insert source");
			current_source_map[primary_neuron.toHICANNOnWafer()]
			                  [primary_neuron.toNeuronOnHICANN()] = ptr;
		}
	}

	auto first = getManager().begin_allocated();
	auto last  = getManager().end_allocated();

	// Note, this works only if ALL threads use the same global population map
	// instance.
	auto start = std::chrono::system_clock::now();
	//tbb::parallel_for_each(first, last,
	std::for_each(first, last,
		[&](HICANNGlobal const& hicann) {
			chip_type& chip = getHardware()[hicann];
			HICANNTransformator trafo(getGraph(), chip, mPyMarocco, mDuration);
			trafo.run(current_source_map[hicann], placement, routing);
		});
	auto end = std::chrono::system_clock::now();
	mPyMarocco.stats.timeSpentInParallelRegion +=
		std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count();


	// FIXME: return default result
	std::unique_ptr<result_type> result(new Result);
	return result;
} // run()



HICANNTransformator::HICANNTransformator(
		graph_t const& graph,
		chip_type& chip,
		pymarocco::PyMarocco& pym,
		double duration
		) :
	mChip(chip),
	mGraph(graph),
	mPyMarocco(pym),
	mDuration(duration)
{}

HICANNTransformator::~HICANNTransformator()
{
	// finally, we need to give sthal the spikes. Note, that we don't have to
	// sort them before hand, because they have to be reordered at the sthal
	// level anyway. This is also why it makes no sense to move (by-rvalue) the
	// spikes to sthal.
	for (auto const& merger : iter_all<dnc_merger_coord>())
	{
		auto const& spikes = mSpikes[merger];
		if (!spikes.empty()) {
			chip().sendSpikes(merger, spikes);
		}
	}
}

std::unique_ptr<HICANNTransformator::result_type>
HICANNTransformator::run(
	CurrentSources const& cs,
	placement::Result const& placement,
	routing::Result const& routing)
{
	auto const& neuron_placement = placement.neuron_placement;

	// assuming that neurons are always read out
	bool const local_neurons = placement.address_assignment.at(chip().index()).has_output();

	bool const local_routes  = routing.crossbar_routing.exists(chip().index());

	// spike input sources
	spike_input(neuron_placement);

	// switch on BackgroundGenerators for locking
	background_generators(mPyMarocco.bkg_gen_isi);

	// load calibration data from DB
	// const auto calib = getCalibrationData();
	// FIXME: get const calibration not possible, because we need to set speedup. see #1543
	auto calib = getCalibrationData();

	// v reset for all FG blocks in bio mV
	double v_reset = 0;

	if (local_neurons)
	{
		//const auto& neuron_calib = calib->atNeuronCollection();
		// FIXME: get const calibration not possible, because we need to set speedup. see #1543
		auto neuron_calib = calib->atNeuronCollection();
		neuron_calib->setSpeedup(mPyMarocco.speedup);
		auto const& synapse_routing = routing.synapse_routing.at(chip().index());

		// Analog output: set correct values for output buffer
		// FIXME: in principle this has nothing to with whether or not there are
		// local neurons. There are other analog output sources than membrane
		// voltages.
		analog_output(*neuron_calib, neuron_placement);

		{
			// 3. transform individual analog parameters
			v_reset =
			    neurons(*neuron_calib, neuron_placement, synapse_routing.synapse_target_mapping);

			if(local_routes) {
				// transform synapses
				auto synapse_row_calib = calib->atSynapseRowCollection();

				// FIXME: remove next lines when synapse calibration exists for real hardware (#1584)
				if ( mPyMarocco.param_trafo.use_ess_synapse_trafo )
					synapse_row_calib->setEssDefaults();
				else
					synapse_row_calib->setDefaults();

				synapses(*synapse_row_calib, synapse_routing, neuron_placement);
			}

			// current sources
			current_input(*neuron_calib, cs);

		}
	}

	if (local_neurons || local_routes)
	{
		const auto& shared_calib = calib->atBlockCollection();

		// transforms global analog parameters
		// this needs to be done, for any HICANN used in any way. For
		// example, shared parameter also control L1.
		//
		// v_reset is in pynn units, i.e. mV, shared_parameters takes
		// HW units, i.e. Volts, shift_v is also given in Volts

		double const mV_to_V = 1/1000.;

		shared_parameters(getGraph(), *shared_calib,
		                  v_reset * mPyMarocco.param_trafo.alpha_v * mV_to_V +
						  mPyMarocco.param_trafo.shift_v);
	}

	return std::unique_ptr<result_type>(new Result);
} // run()

void HICANNTransformator::neuron_config(neuron_calib_t const& /*unused*/)
{

	chip().use_big_capacitors(mPyMarocco.param_trafo.use_big_capacitors);

	// use defaults so far
	//auto& config = chip().neurons.config;
	//config.bigcap         = 0x0;
	//config.slow_I_radapt  = 0x0;
	//config.fast_I_radapt  = 0x0;
	//config.slow_I_gladapt = 0x0;
	//config.fast_I_gladapt = 0x0;
	//config.slow_I_gl      = 0x0;
	//config.fast_I_gl      = 0x0;
}

double HICANNTransformator::neurons(
	neuron_calib_t const& calib,
	typename placement::NeuronPlacementResult const& neuron_placement,
	routing::SynapseTargetMapping const& synapse_target_mapping)
{
	// GLOBAL DIGITAL Neuron Paramets
	neuron_config(calib);

	auto const hicann = chip().index();

	/* SHARED Analog Neuron Parameteres
	 * For each group of neurons that share analog values we have to agree
	 * on common values.  This has to happen prior to the configuration of
	 * individual neuron parameters as these depend on the shared values.
	 *
	 */
	NeuronSharedParameterRequirements shared_parameter_visitor;
	for (auto const& nb : iter_all<NeuronBlockOnHICANN>()) {
		for (auto const& item : neuron_placement.find(NeuronBlockOnWafer(nb, hicann))) {
			auto const& params = getGraph()[item.population()]->parameters();
			for (NeuronOnHICANN nrn : item.logical_neuron()) {
				visitCellParameterVector(
				    params, shared_parameter_visitor, item.neuron_index(), nrn);
			}
		}
	}

	// INDIVIDUAL Neuron Parameters
	TransformNeurons visitor{mPyMarocco.param_trafo.alpha_v, mPyMarocco.param_trafo.shift_v};

	MAROCCO_INFO("Configuring neuron parameters");
	for (auto const& nb : iter_all<NeuronBlockOnHICANN>()) {
		for (auto const& item : neuron_placement.find(NeuronBlockOnWafer(nb, hicann))) {
			auto const& pop = *(getGraph()[item.population()]);
			auto const& logical_neuron = item.logical_neuron();

			// Configure ANALOG neuron parameters.
			for (NeuronOnWafer nrn : logical_neuron) {
				MAROCCO_DEBUG("configuring analog parameters for " << nrn);

				transform_analog_neuron(
					calib, pop, item.neuron_index(), nrn, synapse_target_mapping,
					visitor, chip());
			}

			// As all denmems of a logical neuron will be connected,
			// DIGITAL neuron parameters are only configured for the first denmem.

			NeuronOnHICANN const nrn = logical_neuron.front();
			HMF::HICANN::Neuron& neuron = chip().neurons[nrn];

			// Set L1 address
			auto const& address = item.address();
			assert(address != boost::none);
			MAROCCO_DEBUG(nrn << " has sending address " << address->toL1Address());
			neuron.address(address->toL1Address());
			neuron.activate_firing(true);
			neuron.enable_spl1_output(true);

			// Connect all denmems belonging to this logical neuron.
			assert(logical_neuron.is_rectangular());
			connect_denmems(nrn, logical_neuron.size());
		}
	} // all neuron blocks

	auto const v_resets = shared_parameter_visitor.get_v_resets();
	auto const mean_v_reset = shared_parameter_visitor.get_mean_v_reset();

	if(v_resets.size() != 1) {
		MAROCCO_WARN("more than one value for V_reset requested on " << chip());
		MAROCCO_WARN("only the mean v_reset will be used: " << mean_v_reset << " mV");
		for(auto v_reset : v_resets) {
			MAROCCO_DEBUG("individual v_reset values: " << v_reset << " mV");
		}

	}

	return mean_v_reset;

}

void HICANNTransformator::connect_denmems(
	NeuronOnHICANN const& topleft_neuron,
	size_t hw_neurons_size)
{
	size_t const xwidth = hw_neurons_size/2;
	size_t const xmin   = topleft_neuron.x();
	chip().connect_denmems(X(xmin), X(xmin+xwidth-1));
}


void HICANNTransformator::analog_output(
	neuron_calib_t const& calib, typename placement::NeuronPlacementResult const& neuron_placement)
{
	AnalogVisitor visitor;

	for (auto const& nb : iter_all<NeuronBlockOnHICANN>()) {
		for (auto const& item : neuron_placement.find(NeuronBlockOnWafer(nb, chip().index()))) {
			auto const& pop = *(getGraph()[item.population()]);
			NeuronOnHICANN const nrn = *(item.logical_neuron().begin());
			transform_analog_outputs(
				calib, pop, item.neuron_index(), nrn, visitor, chip());
		}
	}
}

void HICANNTransformator::current_input(neuron_calib_t const& /*calib*/, CurrentSources const& cs)
{
	if (cs.empty()) {
		return;
	} else if (cs.size()>1) {
		MAROCCO_WARN("only one current input per HICANN");
	}

	for (auto const& e : cs)
	{
		NeuronOnHICANN const& nrn = e.first;
		StepCurrentSource const& src = *(e.second);

		auto const& times = src.times();
		auto const& amps = src.amplitudes();

		if (times.size() != amps.size()) {
			throw std::runtime_error(
				"number of amplitudes doesn't match times for StepCurrentInput");
		}

		sthal::FGStimulus pattern;
		static size_t const MAX = pattern.size();
		for (size_t ii = 0; ii < MAX; ii++)
		{
			pattern[ii] = amps[ii*(amps.size()/MAX)];
		}

		pattern.setPulselength(15);
		pattern.setContinuous(true);

		MAROCCO_DEBUG("setting FGStim for " << nrn);
		chip().setCurrentStimulus(nrn, pattern);

		// only one current input available
		return;
	}
}

void HICANNTransformator::background_generators(uint32_t isi)
{
	// configure ALL BackgroundGenerators for Repeater & SynapseDriver locking.
	// They are NOT use for production neuron stimulation.
	for (auto const addr : iter_all<BackgroundGeneratorOnHICANN>()) {
		HMF::HICANN::BackgroundGenerator bg;
		bg.enable(true);
		bg.seed(0);
		bg.address(HMF::HICANN::L1Address(0));
		bg.set_mode(false /*random*/, isi /*isi*/);

		chip().layer1[addr] = bg;
	}
}

void HICANNTransformator::spike_input(
	placement::NeuronPlacementResult const& neuron_placement)
{
	HICANNOnWafer const hicann = chip().index();
	for (auto const dnc_merger : iter_all<DNCMergerOnHICANN>()) {
		for (auto const& item : neuron_placement.find(DNCMergerOnWafer(dnc_merger, hicann))) {
			if (!is_source(item.population(), getGraph())) {
				continue;
			}
			auto const& address = item.address();
			assert(address != boost::none);

			SpikeInputVisitor visitor(
			    mPyMarocco, mSpikes[dnc_merger], int(dnc_merger) * 209823 /*seed*/, mDuration);

			Population const& pop = *(getGraph()[item.population()]);

			// configure input spike parameters
			transform_input_spikes(
			    pop, address->toL1Address(), item.neuron_index(), chip(), visitor);
		}
	}
}

void HICANNTransformator::shared_parameters(
	graph_t const& /*graph*/,
	shared_calib_t const& calib,
	double v_reset)
{
	using namespace HMF;

	for (size_t ii=0; ii<HMF::Coordinate::FGBlockOnHICANN::enum_type::size; ++ii)
	{
		// default values for other parameters are also retrieved
		auto hwparams = calib.applySharedCalibration(v_reset, ii);
		FGBlockOnHICANN fgb{Enum{ii}};
		auto& fg = chip().floating_gates;
		hwparams.toHW(fgb, fg);
	}
}

void HICANNTransformator::synapses(
	synapse_row_calib_t const& calib,
	typename routing::synapse_driver_mapping_t::result_type const&
		synapse_routing, // TODO change this to pass std::vector<DriverResult>
	typename placement::NeuronPlacementResult const& neuron_placement)
{
	NeuronOnHICANNPropertyArray<double> const weight_scale = weight_scale_array( neuron_placement );

	for (routing::DriverResult const& driver_res : synapse_routing.driver_result) {
		for (auto synrow : driver_res.rows() ) {
			auto const & synrow_addr = synrow.first;
			auto const & synrow_source = synrow.second;
			auto const & synapse_mapping = synrow_source.synapses();  // array of synapse mapping (1 row)
			auto synrow_proxy = chip().synapses[synrow_addr];

			std::array<double , NeuronOnHICANN::x_type::size> scaled_weights{{}}; // array storing scaled weights in nanoSiemens, 0. if not used
#ifndef MAROCCO_NDEBUG
			std::array<double , NeuronOnHICANN::x_type::size> bio_weights{{}}; // array storing bio weights in micro Siemens, 0. if not used
#endif // MAROCCO_NDEBUG

			// individual synapses
			for (size_t col = 0; col < synapse_mapping.size(); ++col) {
				routing::SynapseSource const& synapse_source = synapse_mapping[col];
				if (!synapse_source.empty()) {
					// get bio weight
					auto & proj_view = synapse_source.projection_view();
					double bio_weight = proj_view->getWeights()(synapse_source.src(), synapse_source.tgt()); // getWeights returns just a view, no copy

					// get weight scale
					SynapseOnHICANN syn_addr(synrow_addr, SynapseColumnOnHICANN(col));
					double const w_scale = weight_scale[ syn_addr.toNeuronOnHICANN() ];
					assert ( w_scale > 0. ); // check for inconsistency between routing and placement

					// scale and transform
					scaled_weights[col] = bio_weight*w_scale*1000. /*uS to nS*/;

#ifndef MAROCCO_NDEBUG
					// DEBUG
					bio_weights[col] = bio_weight;
#endif // MAROCCO_NDEBUG
				}
			} // all synapses of a row

			// compute max weight and find best gmax configuration
			double max_weight =  * (std::max_element(scaled_weights.cbegin(), scaled_weights.cend()));

			// get copy and not const ref, because findBestGmaxConfig is not const.
			HMF::SynapseRowCalibration row_calib = * boost::dynamic_pointer_cast< HMF::SynapseRowCalibration const > ( calib.at(synrow_addr) );

			HMF::GmaxConfig gc = row_calib.findBestGmaxConfig(max_weight);
			auto const& synapse_trafo = row_calib.at(gc);

			for (size_t col = 0; col < scaled_weights.size(); ++col) {
				if (scaled_weights[col] > 0.) {
					const double scaled_weight = scaled_weights[col];
					const HICANN::SynapseWeight hwweight = synapse_trafo->getDigitalWeight(scaled_weight);
					// store weight
					synrow_proxy.weights [col] = hwweight;

#ifndef MAROCCO_NDEBUG
					SynapseOnHICANN syn_addr(synrow_addr, SynapseColumnOnHICANN(col));
					double clipped_weight = synapse_trafo->getAnalogWeight(hwweight);
					clipped_weight = clipped_weight/weight_scale[syn_addr.toNeuronOnHICANN()]/1000. /*nS to uS*/;
					MAROCCO_DEBUG("synapse weight of " << syn_addr << " set to " << hwweight
						<< ", bio weight " << bio_weights[col] << ", clipped bio weight " << clipped_weight);
#endif // MAROCCO_NDEBUG
				}
			}

			// store gmax config
			auto& driver = chip().synapses[synrow_addr.toSynapseDriverOnHICANN()];
			HICANN::RowConfig& config = driver[synrow_addr.toRowOnSynapseDriver()];

			// selects 1 of 4 V_max values from global FGs
			config.set_gmax(gc.get_sel_Vgmax());

			// gmax divider [1..15]
			// use same config for left and right synaptic input
			config.set_gmax_div(left,  gc.get_gmax_div() );
			config.set_gmax_div(right, gc.get_gmax_div() );
		} // all rows assigned to an input
	} // all inputs
	// TODO: store V_gmax, depending on ESS or not ESS
}

NeuronOnHICANNPropertyArray<double> HICANNTransformator::weight_scale_array(
	typename placement::NeuronPlacementResult const& neuron_placement) const
{
	CMVisitor const cm_visitor{};
	NeuronOnHICANNPropertyArray<double> rv;

	//initialize all values to 0.
	for (auto const& noh : iter_all<NeuronOnHICANN>())
		rv[noh] = 0.;

	auto const& use_bigcap = chip().neurons.config.bigcap;


	for (auto const& nb : iter_all<NeuronBlockOnHICANN>()) {
		// We need to calculate the scaling factor for each logical neuron.
		for (auto const& item : neuron_placement.find(NeuronBlockOnWafer(nb, chip().index()))) {
			auto const& params = getGraph()[item.population()]->parameters();
			auto const& logical_neuron = item.logical_neuron();

			// Sum up the capacity of the connected denmems on the hardware.
			double cm_hw = 0.;
			std::vector<NeuronOnHICANN> connected_neurons;
			connected_neurons.reserve(logical_neuron.size());
			for (NeuronOnHICANN nrn : item.logical_neuron()) {
				// We have to consider different capacitor choices on top / bottom neuron blocks.
				cm_hw +=
				    use_bigcap[nrn.y()] ? NeuronCalibration::big_cap : NeuronCalibration::small_cap;
				connected_neurons.push_back(nrn);
			}

			double const cm_bio = visitCellParameterVector(params, cm_visitor, item.neuron_index());
			double const scale = mPyMarocco.speedup * cm_hw / cm_bio;

			for (auto cnrn : connected_neurons) {
				rv[cnrn] = scale;
			}
		}
	}
	return rv;
}

boost::shared_ptr<HICANNTransformator::calib_t>
HICANNTransformator::getCalibrationData()
{
	using pymarocco::PyMarocco;

	MAROCCO_DEBUG("Hardware backend: " << int(mPyMarocco.backend));
	MAROCCO_DEBUG("Calibration backend: " << int(mPyMarocco.calib_backend));

	boost::shared_ptr<calib_t> calib(new calib_t);

	if (mPyMarocco.backend == PyMarocco::Backend::ESS &&
	    mPyMarocco.calib_backend != PyMarocco::CalibBackend::Default)
		throw std::runtime_error(
		    "Using the ESS with calib_backend != CalibBackend::Default is currently not supported");

	switch(mPyMarocco.calib_backend) {

		case PyMarocco::CalibBackend::XML: {
			calibtic::MetaData md;
			auto backend = getCalibticBackend();

			const int hicann_id = chip().index().toHICANNOnWafer().id().value();
			std::stringstream calib_file;
			calib_file << "w" << int(chip().index().toWafer()) << "-h";
			calib_file << hicann_id;
			const std::string calib_file_string = calib_file.str();

			MAROCCO_INFO("loading calibration file: " << mPyMarocco.calib_path+"/"+calib_file_string << ".xml");
			backend->load(calib_file_string, md, *calib);

			break;
		}

		case PyMarocco::CalibBackend::Default: {
			calib->setDefaults();
			break;
		}

		default:
			throw std::runtime_error("unknown calibration backend type");

	} // switch calib backend


	if(calib->getPLLFrequency() != mPyMarocco.pll_freq) {
		MAROCCO_WARN("PLL stored in HICANNCollection "
		             << int(calib->getPLLFrequency()/1e6) << " MHz != "
		             << int(mPyMarocco.pll_freq/1e6) << " MHz set here.");
	}

	return calib;
}

boost::shared_ptr<calibtic::backend::Backend>
HICANNTransformator::getCalibticBackend()
{
	using namespace calibtic;
	using namespace calibtic::backend;

	switch(mPyMarocco.calib_backend) {

	case pymarocco::PyMarocco::CalibBackend::XML: {
		auto lib = loadLibrary("libcalibtic_xml.so");
		auto backend = loadBackend(lib);

		if (!backend) {
			throw std::runtime_error("unable to load xml backend");
		}

		std::string calib_path = mPyMarocco.calib_path;
		if (std::getenv("MAROCCO_CALIB_PATH") != nullptr) {
			if (!calib_path.empty())
				// we break hard, if the user specified via both ways...
				throw std::runtime_error(
					"colliding settings: environment variable and pymarocco.calib_path both set");
			calib_path = std::string(std::getenv("MAROCCO_CALIB_PATH"));
		}

		backend->config("path", calib_path); // search in calib_path for calibration xml files
		backend->init();
		return backend;
	}

	default:

		throw std::runtime_error("unknown backend type");

	}

}

graph_t const& HICANNTransformator::getGraph() const
{
	return mGraph;
}

HICANNTransformator::chip_type&
HICANNTransformator::chip()
{
	return mChip;
}

HICANNTransformator::chip_type const&
HICANNTransformator::chip() const
{
	return mChip;
}

} // namespace parameter
} // namespace marocco
