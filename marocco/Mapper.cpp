#include <memory>
#include <unordered_map>
#include <chrono>

#include "halco/common/iter_all.h"
#include "halco/hicann/v2/hicann.h"

#include "marocco/resource/BackendLoaderCalib.h"

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

namespace calibtic {
namespace backend {
class XMLBackend;
class BinaryBackend;
}
}

namespace marocco {

using namespace euter;

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

// check layer 1 configuration for disagreement with marocco's assumptions
bool check_layer1(sthal::Layer1 const& l1)
{
	using namespace halco::hicann::v2;

	log4cxx::LoggerPtr const logger = log4cxx::Logger::getLogger("marocco");

	// count number of any background generator send to a dnc merger
	halco::common::typed_array<size_t, DNCMergerOnHICANN> bkg_gen_counts = {0};

	// count on how many dnc mergers a neuron block is send to
	halco::common::typed_array<size_t, NeuronBlockOnHICANN> nb_counts = {0};

	// flag if a DNCMerger receives neuron or external events but no background
	halco::common::typed_array<bool, DNCMergerOnHICANN> has_neurons_or_gbit_but_no_bkg = {false};

	auto const dnc_merger_output = l1.getDNCMergerOutput();
	for (auto const& dnc_merger_coord : halco::common::iter_all<DNCMergerOnHICANN>()) {
		bool has_neurons = false;
		bool has_gbit_input = false;
		bool has_bkg = false;

		for (auto const& o : dnc_merger_output[dnc_merger_coord]) {
			if (boost::get<BackgroundGeneratorOnHICANN>(&o)) {
				++bkg_gen_counts[dnc_merger_coord];
				has_bkg = true;
			} else if (auto* p_nb = boost::get<NeuronBlockOnHICANN>(&o)) {
				++nb_counts[*p_nb];
				has_neurons = true;
			} else if (boost::get<GbitLinkOnHICANN>(&o)) {
				has_gbit_input = true;
			}
		}

		if ((has_neurons || has_gbit_input) && !has_bkg) {
			has_neurons_or_gbit_but_no_bkg[dnc_merger_coord] = true;
		}
	}

	bool any_has_neurons_or_gbit_but_no_bkg = false;

	for (auto const& k : halco::common::iter_all<DNCMergerOnHICANN>()) {
		if (has_neurons_or_gbit_but_no_bkg[k]) {
			LOG4CXX_ERROR(
			    logger,
			    k << " has neurons or external (gbit) input but no enabled background generator");
			any_has_neurons_or_gbit_but_no_bkg = true;
		}
	}

	bool multiple_output_detected = false;

	for (auto const& k : halco::common::iter_all<DNCMergerOnHICANN>()) {
		if (bkg_gen_counts[k] > 1) {
			LOG4CXX_ERROR(
			    logger,
			    k << " receives events from " << bkg_gen_counts[k] << " background generators");
			multiple_output_detected = true;
		}
	}

	for (auto const& k : halco::common::iter_all<NeuronBlockOnHICANN>()) {
		if (nb_counts[k] > 1) {
			// Could be correct if bg generator is used to lock external inputs and
			// L1 addresses of NeuronBlock are not used by the connected buses
			// but not yet implemented so it is checked
			LOG4CXX_ERROR(logger, k << " is send " << nb_counts[k] << " times to L1");
			multiple_output_detected = true;
		}
	}

	return !multiple_output_detected && !any_has_neurons_or_gbit_but_no_bkg;
}

// check configuration for discrepancy with marocco's assumptions
bool check(sthal::Wafer const& w)
{
	for (auto const& hicann_coord : w.getAllocatedHicannCoordinates()) {
		auto const& chip = w[hicann_coord];
		if (!check_layer1(chip.layer1)) {
			return false;
		}
	}
	return true;
}

} // namespace

Mapper::Mapper(
	hardware_type& hw,
	resource_manager_t& mgr,
	resource_fpga_manager_t& fpga_mgr,
	boost::shared_ptr<PyMarocco> const& pymarocco,
	boost::shared_ptr<results::Marocco> const& results)
	: mBioGraph(), mMgr(mgr), mFPGAMgr(fpga_mgr), mHW(hw), mPyMarocco(pymarocco), m_results(results)
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

	auto& graph = mBioGraph.graph();

	size_t neuron_count = num_neurons(graph);
	MAROCCO_INFO(
	    neuron_count << " neurons in " << boost::num_vertices(graph) << " populations");
	m_results->bio_graph = mBioGraph.graph();

	if (mPyMarocco->skip_mapping) {
		// We only need to set up the bio graph when using old results.
		return;
	}

	// rough capacity check; in case we can stop mapping already here
	if (neuron_count > mHW.capacity()) {
		throw std::runtime_error("hardware capacity too low");
	}

	for (auto const hicann : mMgr.present()) {
		MAROCCO_DEBUG("Adding " << hicann << " as an available resource");
		m_results->resources.add(hicann);
	}

	for (auto const fpga : mFPGAMgr.present()) {
		MAROCCO_DEBUG("Adding " << fpga << " as an available resource");
		m_results->resources.add(fpga);
		for (auto const hs_link : halco::common::iter_all<halco::hicann::v2::HighspeedLinkOnDNC>()) {
			if (mFPGAMgr.get(fpga)->hslinks()->has(hs_link)) {
				m_results->resources.add(halco::hicann::v2::HighspeedLinkOnWafer(hs_link, fpga.toDNCOnWafer()));
			}
		}
	}

	// The 3 1/2-steps to complete happiness

	// 1.  P L A C E M E N T
	auto startPlacement = std::chrono::system_clock::now();
	placement::Placement placer(*mPyMarocco, graph, mHW, mMgr);
	auto placement = placer.run(m_results->placement);
	auto endPlacement = std::chrono::system_clock::now();
	getStats().timePlacement =
	    std::chrono::duration_cast<std::chrono::milliseconds>(endPlacement - startPlacement)
	        .count();

	// 2.  R O U T I N G
	auto startRouting = std::chrono::system_clock::now();
	routing::Routing router(
		mBioGraph, mHW, mMgr, *mPyMarocco, m_results->placement);
	router.run(m_results->l1_routing, m_results->synapse_routing);
	auto synapse_loss = router.getSynapseLoss();

	if (synapse_loss) {
		synapse_loss->fill(getStats());
	} else {
		MAROCCO_WARN("no synapse loss data");
	}

	auto endRouting = std::chrono::system_clock::now();
	getStats().timeRouting =
	    std::chrono::duration_cast<std::chrono::milliseconds>(endRouting - startRouting).count();

	if (synapse_loss && !mPyMarocco->continue_despite_synapse_loss &&
	    synapse_loss->getTotalLoss() != 0) {
		MAROCCO_ERROR(getStats());
		throw std::runtime_error("Synapses lost but synapse loss is not accepted. Set "
		                         "PyMarocco continue_despite_synapse_loss to true to "
		                         "continue with loss.");
	}

	// 3.  P A R A M E T E R   T R A N S L A T I O N
	auto startParameterTranslation = std::chrono::system_clock::now();
	for (auto const& hicann : mMgr.allocated()) {
		auto& chip = mHW[hicann];
		// Speedup of calib is adjusted in parameter transformation see #1542
		auto const& calib = mMgr.loadCalib(hicann);
		parameter::HICANNParameters hicann_parameters(
		    mBioGraph, chip, *mPyMarocco, m_results->placement, placement->merger_routing,
		    m_results->synapse_routing, m_results->parameter, calib, pynn.getDuration());
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
	auto endParameterTranslation = std::chrono::system_clock::now();
	getStats().timeParameterTranslation = std::chrono::duration_cast<std::chrono::milliseconds>(
	                                          endParameterTranslation - startParameterTranslation)
	                                          .count();

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

	bool const check_mapping =
	    (mPyMarocco->scrutinize_mapping == PyMarocco::ScrutinizeMapping::SkipScrutinize)
	        ? true
	        : check(mHW);

	switch (mPyMarocco->scrutinize_mapping) {
		case PyMarocco::ScrutinizeMapping::Scrutinize:
			if (!check_mapping) {
				MAROCCO_ERROR("Mapping checks failed. Set scrutinize_mapping to "
				              "ScrutinizeButIgnore to ignore.");
				throw std::runtime_error("mapping checks failed");
			}
			break;
		case PyMarocco::ScrutinizeMapping::ScrutinizeButIgnore:
			if (!check_mapping) {
				MAROCCO_WARN("mapping checks failed");
			}
			break;
		case PyMarocco::ScrutinizeMapping::SkipScrutinize:
			break;
		default:
			MAROCCO_ERROR("Unknown option for scrutinize_mapping. Choose one of "
			              "Scrutinize, SkipScrutinze or ScrutinizeButIgnore.");
			throw std::runtime_error("Unknown option for scrutinize_mapping.");
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

boost::shared_ptr<results::Marocco> Mapper::results()
{
	return m_results;
}

boost::shared_ptr<results::Marocco const> Mapper::results() const
{
	return m_results;
}

} // namespace marocco
