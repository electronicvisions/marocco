#include <memory>
#include <unordered_map>
#include <chrono>

#include "calibtic/backend/Backend.h"
#include "calibtic/backend/Library.h"

#include "marocco/HardwareUsage.h"
#include "marocco/Logger.h"
#include "marocco/Mapper.h"
#include "marocco/Result.h"
#include "marocco/parameter/AnalogOutputs.h"
#include "marocco/parameter/CurrentSources.h"
#include "marocco/parameter/SpikeTimes.h"
#include "marocco/parameter/HICANNParameters.h"
#include "marocco/placement/Placement.h"
#include "marocco/routing/Routing.h"
#include "marocco/routing/SynapseLoss.h"
#include "marocco/util/iterable.h"

using namespace pymarocco;

namespace marocco {

namespace {

template<typename Graph>
size_t num_neurons(Graph const& g)
{
	size_t cnt = 0;
	for (auto const& vd : make_iterable(vertices(g))) {
		auto pop = g[vd];
		if (!pop->parameters().is_source()) {
			cnt += pop->size();
		}
	}
	return cnt;
}

boost::shared_ptr<calibtic::backend::Backend> load_calibtic_backend(
    std::string calib_path, pymarocco::PyMarocco::CalibBackend calib_backend) {
	boost::shared_ptr<calibtic::backend::Library> lib;

	switch (calib_backend) {
		case pymarocco::PyMarocco::CalibBackend::XML:
			lib = calibtic::backend::loadLibrary("libcalibtic_xml.so");
			break;
		case pymarocco::PyMarocco::CalibBackend::Binary:
			lib = calibtic::backend::loadLibrary("libcalibtic_binary.so");
			break;
		default:
			throw std::runtime_error("unknown calibration backend type");
	}

	auto backend = calibtic::backend::loadBackend(lib);

	if (!backend) {
		throw std::runtime_error("unable to load calib backend");
	}

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

} // namespace

Mapper::Mapper(
	hardware_type& hw,
	resource_manager_t& mgr,
	boost::shared_ptr<PyMarocco> const& pymarocco,
	boost::shared_ptr<results::Marocco> const& results)
	: mBioGraph(), mMgr(mgr), mHW(hw), mPyMarocco(pymarocco), m_results(results)
{
	if (!mPyMarocco) {
		mPyMarocco = PyMarocco::create();
	}

	if (!m_results) {
		m_results = boost::make_shared<results::Marocco>();
	}
}

void Mapper::run(ObjectStore const& pynn)
{
	auto start = std::chrono::system_clock::now();

	// B U I L D   G R A P H
	mBioGraph.load(pynn);

	// write out bio graph in graphviz format
	if (!mPyMarocco->bio_graph.empty()) {
		MAROCCO_INFO("writing bio graph to " << mPyMarocco->bio_graph);
		mBioGraph.write_graphviz(mPyMarocco->bio_graph);
	}

	auto& graph = mBioGraph.graph();

	size_t neuron_count = num_neurons(graph);
	MAROCCO_INFO(
	    neuron_count << " neurons in " << boost::num_vertices(graph) << " populations");

	if (mPyMarocco->skip_mapping) {
		// We only need to set up the bio graph when using old results.
		return;
	}

	// rough capacity check; in case we can stop mapping already here
	if (neuron_count > mHW.capacity()) {
		throw std::runtime_error("hardware capacity too low");
	}

	for (auto const hicann : mMgr.present()) {
		m_results->resources.add(hicann);
	}

	// The 3 1/2-steps to complete happiness

	// 1.  P L A C E M E N T
	placement::Placement placer(*mPyMarocco, graph, mHW, mMgr);
	auto placement = placer.run(m_results->placement);

	// 2.  R O U T I N G
	routing::Routing router(
		mBioGraph, mHW, mMgr, *mPyMarocco, m_results->placement);
	router.run(m_results->l1_routing, m_results->synapse_routing);
	auto synapse_loss = router.getSynapseLoss();

	if (synapse_loss) {
		synapse_loss->fill(getStats());
	} else {
		MAROCCO_WARN("no synapse loss data");
	}

	if (synapse_loss && !mPyMarocco->continue_despite_synapse_loss &&
	    synapse_loss->getTotalLoss() != 0) {
		MAROCCO_ERROR(getStats());
		throw std::runtime_error("Synapses lost but synapse loss is not accepted. Set "
		                         "PyMarocco continue_despite_synapse_loss to true to "
		                         "continue with loss.");
	}

	// 3.  P A R A M E T E R   T R A N S L A T I O N

	boost::shared_ptr<calibtic::backend::Backend> calib_backend;
	switch (mPyMarocco->calib_backend) {
		case pymarocco::PyMarocco::CalibBackend::XML:
		case pymarocco::PyMarocco::CalibBackend::Binary:
			calib_backend =
			    load_calibtic_backend(mPyMarocco->calib_path, mPyMarocco->calib_backend);
			break;
		case pymarocco::PyMarocco::CalibBackend::Default:
			break;
		default:
			throw std::runtime_error("unknown calibration backend type");
	}

	for (auto const& hicann : mMgr.allocated()) {
		auto& chip = mHW[hicann];
		parameter::HICANNParameters hicann_parameters(
			mBioGraph, chip, *mPyMarocco, m_results->placement, m_results->synapse_routing,
			calib_backend, pynn.getDuration());
		hicann_parameters.run();
	}

	// collect current sources
	parameter::CurrentSources::current_sources_type bio_current_sources;

	for (auto const& current_source : pynn.current_sources()) {
		auto target_assembly = current_source->target();
		for (auto population_view : target_assembly->populations()) {
			auto const& mask = population_view.mask();
			for (size_t ii = 0; ii < mask.size(); ++ii) {
				if (!mask[ii]) {
					continue;
				}

				auto vertex = mBioGraph[population_view.population_ptr().get()];
				bio_current_sources[BioNeuron(vertex, ii)] = current_source;
			}
		}
	}

	MAROCCO_DEBUG(
		"Transformed " << pynn.current_sources().size() << " current sources into "
		<< bio_current_sources.size() << " single-neuron items");

	auto& wafer_config = mHW;
	parameter::CurrentSources current_sources(wafer_config, m_results->placement);
	current_sources.run(bio_current_sources);

	parameter::AnalogOutputs aouts(mBioGraph, m_results->placement);
	aouts.run(m_results->analog_outputs);

	parameter::SpikeTimes spike_times(mBioGraph, pynn.getDuration());
	spike_times.run(m_results->spike_times);

	auto end = std::chrono::system_clock::now();
	getStats().timeTotal =
		std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count();


	// update stats
	getStats().setNumPopulations(pynn.populations().size());
	getStats().setNumProjections(pynn.projections().size());
	getStats().setNumNeurons(neuron_count);

	// and print them
	MAROCCO_INFO(getStats());

	// generate Hardware stats
	HardwareUsage usage(mHW, mMgr, *placement);
	usage.fill(getStats());
}

Mapper::hardware_type&
Mapper::getHardware()
{
	return mHW;
}

Mapper::hardware_type const&
Mapper::getHardware() const
{
	return mHW;
}

pymarocco::MappingStats& Mapper::getStats()
{
	return mPyMarocco->getStats();
}

pymarocco::MappingStats const& Mapper::getStats() const
{
	return mPyMarocco->getStats();
}

BioGraph const& Mapper::bio_graph() const
{
	return mBioGraph;
}

boost::shared_ptr<results::Marocco> Mapper::results()
{
	return m_results;
}

boost::shared_ptr<results::Marocco const> Mapper::results() const
{
	return m_results;
}

} // namespace marocco
