#include "marocco/parameter/HICANNParameter.h"

#include <tbb/parallel_for_each.h>
#include <algorithm>
#include <memory>
#include <chrono>
#include <string>
#include <sstream>

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

	std::unordered_map<HICANNGlobal, std::unordered_map<NeuronOnHICANN,
		boost::shared_ptr<StepCurrentSource const>>> current_source_map;

	auto const& pm = placement.placement();
	auto const& np = placement.neuron_placement;
	for (auto const& entry : mCurrentSourceMap)
	{
		Vertex const v = entry.first;
		size_t const nrn = entry.second.first;

		auto const mapping = pm.at(v);

		for (auto const& assign : mapping) {
			auto const& terminal = assign.coordinate();
			auto const entry_ptr =
			    np.at(terminal.toHICANNGlobal())[terminal.toNeuronBlockOnHICANN()]
			                                    [assign.offset()];
			if (!entry_ptr) {
				continue;
			}

			auto const& bio = entry_ptr->population_slice();
			if (nrn>=bio.offset() && nrn<bio.offset()+bio.size())
			{
				// winner

				auto ptr = boost::dynamic_pointer_cast<StepCurrentSource const>(entry.second.second);
				if (!ptr) {
					MAROCCO_WARN("unsupported current type");
				} else {
					MAROCCO_DEBUG("insert source");
					auto n = assign.offset().toNeuronOnHICANN(
					    terminal.toNeuronBlockOnHICANN());
					current_source_map[terminal.toHICANNGlobal()][n] = ptr;
				}

				break;
			}
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

	// assuming that neurons are always read out
	auto const& output_mapping = placement.output_mapping.at(chip().index());
	bool const local_neurons = !output_mapping.onlyInput();

	bool const local_routes  = routing.crossbar_routing.exists(chip().index());

	// spike input sources
	spike_input(output_mapping);

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
		auto const& neuron_placement = placement.neuron_placement.at(chip().index());
		auto const& synapse_routing = routing.synapse_routing.at(chip().index());

		// Analog output: set correct values for output buffer
		// FIXME: in principle this has nothing to with whether or not there are
		// local neurons. There are other analog output sources than membrane
		// voltages.
		analog_output(*neuron_calib, neuron_placement);

		bool const has_output_mapping = output_mapping.any();
		if (has_output_mapping)
		{
			// 3. transform individual analog parameters
			v_reset = neurons(*neuron_calib, neuron_placement, output_mapping,
			                  synapse_routing.synapse_target_mapping);

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
	typename placement::neuron_placement_t::result_type const& neuron_placement,
	typename placement::output_mapping_t::result_type const& output_mapping,
	routing::SynapseTargetMapping const& synapse_target_mapping)
{
	// GLOBAL DIGITAL Neuron Paramets
	neuron_config(calib);

	// set Neuron sending L1Addresses
	typedef std::vector<HMF::HICANN::L1Address>::const_iterator It;
	std::unordered_map<assignment::PopulationSlice, std::pair<It, It>> address_map;
	for (auto const& merger : iter_all<DNCMergerOnHICANN>())
	{
		// skip if this is an input assignment
		if (output_mapping.getMode(merger) != placement::OutputBufferMapping::OUTPUT) {
			continue;
		}

		auto const& list = output_mapping.at(merger);
		for (auto const& am : list)
		{
			auto const& addr = am.addresses();
			auto const& bio = am.bio();
			auto res = address_map.emplace(bio,
				std::make_pair(addr.cbegin(), addr.cend())); // FIXME: unused
			static_cast<void>(res);
		}
	}

	/* SHARED Analog Neuron Parameteres
	 * For each group of neurons that share analog values we have to agree
	 * on common values.  This has to happen prior to the configuration of
	 * individual neuron parameters as these depend on the shared values.
	 *
	 */
	NeuronSharedParameterRequirements shared_parameter_visitor;
	for (auto const& nb : iter_all<NeuronBlockOnHICANN>()) {
		placement::OnNeuronBlock const& onb = neuron_placement[nb];

		for (auto it = onb.begin(); it != onb.end(); ++it) {
			assignment::PopulationSlice const& bio = (*it)->population_slice();
			Population const& pop = *(getGraph()[bio.population()]);

			size_t const hw_neuron_size = (*it)->neuron_size();
			auto const& params = pop.parameters();
			for (auto& neuron : chunked(onb.neurons(it), hw_neuron_size)) {
				for (NeuronOnNeuronBlock nrn : neuron) {
					visitCellParameterVector(
						params, shared_parameter_visitor, bio.offset() + neuron.index(),
						nrn.toNeuronOnHICANN(nb));
				}
			}
		}
	}

	// INDIVIDUAL Neuron Parameters
	TransformNeurons visitor{mPyMarocco.param_trafo.alpha_v, mPyMarocco.param_trafo.shift_v};

	for (auto const& nb : iter_all<NeuronBlockOnHICANN>())
	{
		placement::OnNeuronBlock const& onb = neuron_placement[nb];

		for (auto it = onb.begin(); it != onb.end(); ++it) {
			assignment::PopulationSlice const& bio = (*it)->population_slice();
			Population const& pop = *(getGraph()[bio.population()]);

			It first, last;
			std::tie(first, last) = address_map.at(bio);

			size_t const hw_neuron_size = (*it)->neuron_size();

			for (auto& neuron : chunked(onb.neurons(it), hw_neuron_size)) {
				for (NeuronOnNeuronBlock nrn_onb : neuron) {
					auto const nrn = nrn_onb.toNeuronOnHICANN(nb);

					MAROCCO_INFO("configuring neuron: " << nrn);

					// configure ANALOG neuron parameters
					transform_analog_neuron(
						calib, pop, bio.offset() + neuron.index(), nrn, synapse_target_mapping,
						visitor, chip());
				}

				{
					// As hardware neurons will be connected, DIGITAL
					// neuron parameters are only configured for the first
					// hardware neuron of each bio neuron.
					auto const nrn = neuron.begin()->toNeuronOnHICANN(nb);
					HMF::HICANN::Neuron& neuron = chip().neurons[nrn];

					if (first == last) {
						throw std::runtime_error("out of L1Addresses");
					}

					// set neuron l1 address
					// FIXME: Document this assertion. see MergerRouting?
					if (neuron.address() != *first++) {
						throw std::runtime_error("neuron L1Address mismatch");
					}

					MAROCCO_INFO(chip().index() << " " << nb << " " << nrn
					                            << " has sending address "
					                            << neuron.address());

					if (!neuron.activate_firing()) {
						throw std::runtime_error("neuron has disabled spike mechanism");
					}

					if (!neuron.enable_spl1_output()) {
						MAROCCO_WARN(nrn << " " << neuron.address() << " is muted");
					}

					connect_denmems(nrn, hw_neuron_size);
				}
			}
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
	neuron_calib_t const& calib,
	typename placement::neuron_placement_t::result_type const& neuron_placement)
{
	AnalogVisitor visitor;

	for (auto const& nb : iter_all<NeuronBlockOnHICANN>())
	{
		placement::OnNeuronBlock const& onb = neuron_placement[nb];

		for (auto it = onb.begin(); it != onb.end(); ++it) {
			assignment::PopulationSlice const& bio = (*it)->population_slice();
			Population const& pop = *(getGraph()[bio.population()]);

			size_t const hw_neuron_size = (*it)->neuron_size();

			// FIXME: Not necessary anymore, size is checked in NeuronPlacement ctor
			if (hw_neuron_size == 0) {
				throw std::runtime_error("hw neuron size must be >0");
			}

			for (auto& neuron : chunked(onb.neurons(it), hw_neuron_size)) {
				auto const nrn = neuron.begin()->toNeuronOnHICANN(nb);
				transform_analog_outputs(
					calib, pop, bio.offset() + neuron.index(), nrn, visitor, chip());
			}
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
	placement::OutputBufferMapping const& output_mapping)
{
	// iterate over the 8 output buffer
	for (auto const& merger : iter_all<DNCMergerOnHICANN>())
	{
		if (output_mapping.getMode(merger) == placement::OutputBufferMapping::OUTPUT) {
			// i.e. dnc merger is not receiving
			continue;
		}
		SpikeInputVisitor visitor(mPyMarocco, mSpikes[merger],
				int(merger)*209823 /*seed*/, mDuration);

		for (assignment::AddressMapping const& am: output_mapping.at(merger))
		{
			assignment::PopulationSlice const& bio = am.bio();
			if (!is_source(bio.population(), getGraph()))
				throw std::runtime_error(
				    "real neurons (i.e. it's not a source!) assigned to input OutputBuffers");

			Population const& pop = *(getGraph()[bio.population()]);

			// for all inputs (== input population size)
			for (size_t n=0; n<bio.size(); ++n)
			{
				HMF::HICANN::L1Address const& l1 = am.addresses().at(n);

				// configure input spike parameters
				transform_input_spikes(
					pop, l1, bio.offset()+n,
					chip(), visitor);
			}
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
	typename placement::neuron_placement_t::result_type const& neuron_placement)
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
	typename placement::neuron_placement_t::result_type const& neuron_placement
	) const {

	CMVisitor const cm_visitor{};
	NeuronOnHICANNPropertyArray<double> rv;

	//initialize all values to 0.
	for (auto const& noh : iter_all<NeuronOnHICANN>())
		rv[noh] = 0.;

	auto const& use_bigcap = chip().neurons.config.bigcap;

	for (auto const& nb : iter_all<NeuronBlockOnHICANN>())
	{
		placement::OnNeuronBlock const& onb = neuron_placement[nb];

		for (auto it = onb.begin(); it != onb.end(); ++it) {
			assignment::PopulationSlice const& bio = (*it)->population_slice();
			Population const& pop = *(getGraph()[bio.population()]);

			double cm_bio = 0.;
			double cm_hw = 0.;
			size_t const hw_neuron_size = (*it)->neuron_size();
			std::vector<NeuronOnHICANN> connected_neurons;
			connected_neurons.reserve(hw_neuron_size);

			auto const& params = pop.parameters();

			// iterate over all hardware neurons of the NeuronPlacement.
			for (auto& neuron : chunked(onb.neurons(it), hw_neuron_size)) {
				{
					// for the first hw neuron of a bio neuron: get the bio cap
					cm_bio =
						visitCellParameterVector(params, cm_visitor, bio.offset() + neuron.index());
				}

				for (NeuronOnNeuronBlock nrn : neuron) {
					// sum up the hw cap and collect the connected denmems
					// This can even handle different capacitor choices on the top and bottom
					// blocks.
					cm_hw += use_bigcap[nrn.y()] ? NeuronCalibration::big_cap
					                             : NeuronCalibration::small_cap;
					connected_neurons.push_back(nrn.toNeuronOnHICANN(nb));
				}

				// calculate the scaling factor store the factor for all connected denmems and
				// reset the values
				double scale = mPyMarocco.speedup * cm_hw / cm_bio;
				for (auto cnrn : connected_neurons)
					rv[cnrn] = scale;
				connected_neurons.clear();
				cm_bio = 0.;
				cm_hw = 0.;
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

		case PyMarocco::CalibBackend::DB: {
			try {
				calibtic::MetaData md;
				auto backend = getCalibticBackend();
				backend->load(generateUID(chip(), 0, ""), md, *calib);
			} catch (std::runtime_error const& e) {
				warn(this) << e.what();
				calib->setDefaults();
			}

			break;
		}

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

	case pymarocco::PyMarocco::CalibBackend::DB: {
		auto lib = loadLibrary("libcalibtic_mongo.so");
		auto backend = loadBackend(lib);

		if (!backend) {
			throw std::runtime_error("unable to load mongo backend");
		}

		backend->config("host", "localhost");
		backend->config("port", 27017);
		backend->init();
		return backend;
	}

	case pymarocco::PyMarocco::CalibBackend::XML: {
		auto lib = loadLibrary("libcalibtic_xml.so");
		auto backend = loadBackend(lib);

		if (!backend) {
			throw std::runtime_error("unable to load xml backend");
		}

		backend->config("path", mPyMarocco.calib_path); // search in calib_path for calibration xml files
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
