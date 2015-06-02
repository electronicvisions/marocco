#include <memory>
#include <unordered_map>
#include <chrono>

#include "marocco/Mapper.h"
#include "marocco/GraphBuilder.h"

// algorighms
#include "marocco/Logger.h"
#include "marocco/placement/Placement.h"
#include "marocco/routing/Routing.h"
#include "marocco/routing/SynapseLoss.h"
#include "marocco/parameter/HICANNParameter.h"
#include "marocco/partition/CakePartitioner.h"
#include "marocco/HardwareUsage.h"
#include "marocco/Result.h"

using namespace pymarocco;

namespace marocco {

template<typename Graph>
size_t num_neurons(Graph const& g)
{
	auto const populations = get(population_t(), g);
	size_t cnt = 0;
	for(auto const& vd : make_iterable(vertices(g))) {
			auto pop = get(populations, vd);
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
	mGraph(),
	mMgr(mgr),
	mHW(hw),
	mComm(comm),
	mPyMarocco(pymarocco)
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

	// B U I L D ,   P A R T I T I O N   &   D I S T R I B U T E   G R A P H
	GraphBuilder gb(mGraph, getComm());
	gb.build(pynn);

	// write out bio graph in graphviz format
	if (!mPyMarocco->bio_graph.empty()) {
		info(this) << "writing bio graph to " << mPyMarocco->bio_graph;
		gb.write_bio_graph(mPyMarocco->bio_graph);
	}

	size_t num_local_neurons = num_neurons(mGraph);
	info() << num_local_neurons << " neurons in "
		<< boost::num_vertices(mGraph) << " populations";

	// P A R T I T I O N   H A R D W A R E
	partition::CakePartitioner cake(mMgr, mComm.Get_size(), mComm.Get_rank());
	info() << mMgr.count_present() << " available HICANNs";

	// rough capacity check; in case we can stop mapping already here
	if (num_local_neurons > mHW.capacity())
		throw std::runtime_error("hardware capacity too low");


	// The 3 1/2-steps to complete happiness

	// 1.  P L A C E M E N T
	std::unique_ptr<placement::Placement> placer(
		new placement::DefaultPlacement(*mPyMarocco, mGraph, mHW, mMgr, getComm()));
	auto placement = placer->run();
	mLookupTable = result_cast<placement::Result>(*placement).reverse_mapping;

	// 2.  R O U T I N G
	std::unique_ptr<routing::Routing> router(
		new routing::DefaultRouting(*mPyMarocco, mGraph, mHW, mMgr, getComm()));
	auto routing = router->run(*placement);
	auto synapse_loss = router->getSynapseLoss();

	// 3.  P A R A M E T E R   T R A N S L A T I O N

	// collect current sources
	typedef graph_t::vertex_descriptor Vertex;
	auto const& current_sources = pynn.current_sources();
	auto const& vertex_mapping = gb.vertex_mapping();
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
					Vertex v = vertex_mapping.at(view.population_ptr().get());
					mSources[v] = std::make_pair(ii, source);
				}
			}
		}
	}
	MAROCCO_DEBUG("current_sources.size(): " << current_sources.size());
	MAROCCO_DEBUG("mSources.size(): " << mSources.size());

	std::unique_ptr<parameter::HICANNParameter> transformator(
		new parameter::HICANNParameter(*mPyMarocco, mSources,
			pynn.getDuration(), mGraph, mHW, mMgr, getComm()));
	auto parameter = transformator->run(*placement, *routing);


	auto end = std::chrono::system_clock::now();
	getStats().timeTotal =
		std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count();


	// update stats
	getStats().setNumPopulations(pynn.populations().size());
	getStats().setNumProjections(pynn.projections().size());
	getStats().setNumNeurons(num_local_neurons);

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
}

Mapper::graph_type&
Mapper::getNeuralNetwork()
{
	return mGraph;
}

Mapper::graph_type const&
Mapper::getNeuralNetwork() const
{
	return mGraph;
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

std::shared_ptr<placement::LookupTable>
Mapper::getLookupTable() const
{
	return mLookupTable;
}

} // namespace marocco
