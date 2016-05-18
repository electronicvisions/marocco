#include <memory>
#include <unordered_map>
#include <chrono>

#include "marocco/HardwareUsage.h"
#include "marocco/Logger.h"
#include "marocco/Mapper.h"
#include "marocco/Result.h"
#include "marocco/parameter/AnalogOutputs.h"
#include "marocco/parameter/CurrentSources.h"
#include "marocco/parameter/HICANNParameters.h"
#include "marocco/placement/Placement.h"
#include "marocco/routing/Routing.h"
#include "marocco/routing/SynapseLoss.h"
#include "marocco/util/guess_wafer.h"
#include "marocco/util/iterable.h"

using namespace pymarocco;

namespace marocco {

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

Mapper::Mapper(hardware_type& hw,
			   resource_manager_t& mgr,
			   boost::shared_ptr<PyMarocco> const& pymarocco) :
	mBioGraph(),
	mMgr(mgr),
	mHW(hw),
	mPyMarocco(pymarocco),
	m_results(new results::Marocco())
{
	if (!mPyMarocco) {
		mPyMarocco = PyMarocco::create();
	}
}

void Mapper::run(ObjectStore const& pynn)
{
	auto start = std::chrono::system_clock::now();

	// B U I L D   G R A P H
	mBioGraph.load(pynn);

	// write out bio graph in graphviz format
	if (!mPyMarocco->bio_graph.empty()) {
		info(this) << "writing bio graph to " << mPyMarocco->bio_graph;
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

	// The 3 1/2-steps to complete happiness

	// 1.  P L A C E M E N T
	placement::Placement placer(*mPyMarocco, graph, mHW, mMgr);
	auto placement = placer.run(m_results->placement);

	// 2.  R O U T I N G
	std::unique_ptr<routing::Routing> router(
		new routing::DefaultRouting(*mPyMarocco, graph, mHW, mMgr));
	auto routing = router->run(*placement);
	auto synapse_loss = router->getSynapseLoss();

	// 3.  P A R A M E T E R   T R A N S L A T I O N

	for (auto const& hicann : mMgr.allocated()) {
		auto& chip = mHW[hicann];
		parameter::HICANNParameters hicann_parameters(
			mBioGraph, chip, *mPyMarocco, result_cast<placement::Result>(*placement),
			result_cast<routing::Result>(*routing), pynn.getDuration());
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

	auto& wafer_config = mHW[guess_wafer(mMgr)];
	parameter::CurrentSources current_sources(wafer_config, m_results->placement);
	current_sources.run(bio_current_sources);

	parameter::AnalogOutputs aouts(mBioGraph, m_results->placement);
	aouts.run(m_results->analog_outputs);

	auto end = std::chrono::system_clock::now();
	getStats().timeTotal =
		std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count();


	// update stats
	getStats().setNumPopulations(pynn.populations().size());
	getStats().setNumProjections(pynn.projections().size());
	getStats().setNumNeurons(neuron_count);

	if (synapse_loss) {
		synapse_loss->fill(getStats());
	} else {
		warn(this) << "no synapse loss data";
	}

	// and print them
	info(this) << getStats();

	// generate Hardware stats
	HardwareUsage usage(getHardware(), mMgr, *placement);
	usage.fill(getStats());

	if (!mPyMarocco->persist.empty()) {
		MAROCCO_INFO("Saving results to " << mPyMarocco->persist);
		m_results->save(mPyMarocco->persist.c_str(), true);
	}
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

results::Marocco& Mapper::results()
{
	return *m_results;
}

results::Marocco const& Mapper::results() const
{
	return *m_results;
}

} // namespace marocco
