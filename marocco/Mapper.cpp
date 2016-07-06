#include <memory>
#include <unordered_map>
#include <chrono>

#include "marocco/HardwareUsage.h"
#include "marocco/Logger.h"
#include "marocco/Mapper.h"
#include "marocco/Result.h"
#include "marocco/parameter/HICANNParameter.h"
#include "marocco/placement/Placement.h"
#include "marocco/routing/Routing.h"
#include "marocco/routing/SynapseLoss.h"
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
			   comm_type const& comm,
			   boost::shared_ptr<PyMarocco> const& pymarocco) :
	mBioGraph(),
	mMgr(mgr),
	mHW(hw),
	mComm(comm),
	mPyMarocco(pymarocco),
	m_results(new results::Marocco())
{
	if (!mPyMarocco) {
		mPyMarocco = PyMarocco::create();
	}
}

void Mapper::run(ObjectStore const& pynn)
{
	// quit early if this process is in the second communicator
	if (MPI::COMM_WORLD.Get_rank() != getComm().Get_rank())
		return;

	// DEBUG output
	if (getComm().Get_rank() == MASTER_PROCESS)
	{
		debug() << pynn;
	}

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

	// collect current sources
	typedef graph_t::vertex_descriptor Vertex;
	auto const& current_sources = pynn.current_sources();
	parameter::HICANNParameter::CurrentSourceMap mSources;

	for (auto const& source : current_sources)
	{
		auto target = source->target();
		for (auto view : target->populations())
		{
			auto const& mask = view.mask();
			for (size_t ii=0; ii<mask.size(); ++ii)
			{
				if (mask[ii]) {
					Vertex v = mBioGraph[view.population_ptr().get()];
					mSources[v] = std::make_pair(ii, source);
				}
			}
		}
	}
	MAROCCO_DEBUG("current_sources.size(): " << current_sources.size());
	MAROCCO_DEBUG("mSources.size(): " << mSources.size());

	std::unique_ptr<parameter::HICANNParameter> transformator(
		new parameter::HICANNParameter(*mPyMarocco, mSources,
			pynn.getDuration(), graph, mHW, mMgr));
	auto parameter = transformator->run(*placement, *routing);


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

Mapper::comm_type&
Mapper::getComm()
{
	return mComm;
}
Mapper::comm_type const&
Mapper::getComm() const
{
	return mComm;
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
