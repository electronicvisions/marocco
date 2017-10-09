#include "marocco/mapping.h"

#include <cstdlib>
#include <memory>
#include <stdexcept>
#include <boost/filesystem.hpp>
#include <boost/make_shared.hpp>

#include "redman/backend/Backend.h"
#include "redman/backend/Library.h"
#include "redman/backend/MockBackend.h"
#include "redman/resources/Wafer.h"
#include "sthal/DontProgramFloatingGatesHICANNConfigurator.h"
#include "sthal/ESSHardwareDatabase.h"
#include "sthal/ESSRunner.h"
#include "sthal/ExperimentRunner.h"
#include "sthal/HICANNConfigurator.h"
#include "sthal/HICANNv4Configurator.h"
#include "sthal/NoResetNoFGConfigurator.h"
#include "sthal/NoFGConfigurator.h"
#include "sthal/ParallelHICANNv4Configurator.h"
#include "sthal/ParallelHICANNNoFGConfigurator.h"
#include "sthal/ParallelHICANNNoResetNoFGConfigurator.h"
#include "sthal/OnlyNeuronNoResetNoFGConfigurator.h"
#include "sthal/VerifyConfigurator.h"
#include "sthal/MagicHardwareDatabase.h"

#include "marocco/Logger.h"
#include "marocco/Mapper.h"
#include "marocco/experiment/AnalogOutputsConfigurator.h"
#include "marocco/experiment/Experiment.h"
#include "marocco/experiment/SpikeTimesConfigurator.h"
#include "pymarocco/PyMarocco.h"
#include "pymarocco/runtime/Runtime.h"

using namespace HMF::Coordinate;

namespace {

boost::shared_ptr<redman::backend::Backend> load_redman_backend(pymarocco::Defects const& defects)
{
	if (defects.backend == pymarocco::Defects::Backend::XML) {
		auto const lib = redman::backend::loadLibrary("libredman_xml.so");
		auto const backend = redman::backend::loadBackend(lib);

		if (!backend) {
			throw std::runtime_error("unable to load xml backend");
		}

		std::string defects_path = defects.path;
		if (std::getenv("MAROCCO_DEFECTS_PATH") != nullptr) {
			if (!defects_path.empty()) {
				throw std::runtime_error(
				    "only one of pymarocco.defects.path and MAROCCO_DEFECTS_PATH "
				    "should be set");
			}
			defects_path = std::string(std::getenv("MAROCCO_DEFECTS_PATH"));
		}

		backend->config("path", defects_path);
		backend->init();
		return backend;
	} else if (defects.backend == pymarocco::Defects::Backend::None) {
		return boost::make_shared<redman::backend::MockBackend>();
	}
	throw std::runtime_error("defects backend not implemented");
}

std::string create_temporary_directory(char const* tpl) {
	using namespace boost::filesystem;
	path tmp_dir;
	bool success = false;
	do {
		tmp_dir = temp_directory_path() / unique_path(tpl);
		success = create_directory(tmp_dir);
	} while (!success);
	return tmp_dir.native();
}

struct DeleteRecursivelyOnScopeExit {
	std::string path;

	~DeleteRecursivelyOnScopeExit()
	{
		namespace bfs = boost::filesystem;
		if (!path.empty()) {
			// Delete recursively if path exists.
			bfs::remove_all(bfs::path(path));
		}
	}
};

} // namespace

namespace marocco {
namespace mapping {

std::set<Wafer> wafers_used_in(boost::shared_ptr<ObjectStore> store)
{
	auto mi = store->getMetaData<pymarocco::PyMarocco>("marocco");

	std::set<Wafer> wafers;

	for (auto const& defect : mi->defects.hicanns()) {
		wafers.insert(defect.first.toWafer());
	}

	return wafers;
}

MappingResult run(boost::shared_ptr<ObjectStore> store) {
	using pymarocco::PyMarocco;

	log4cxx::LoggerPtr const logger = log4cxx::Logger::getLogger("marocco");

	auto mi = store->getMetaData<PyMarocco>("marocco");
	auto wafers = wafers_used_in(store);

	if (wafers.size() > 1) {
		throw std::runtime_error("currently only a single wafer is supported.");
	}

	// When marocco lives inside the same process as pyhmf, the user may pass in a wafer
	// config pointer to work around the overhead of dumping / loading the configuration
	// and connecting / resetting the hardware in “in the loop”-style experiments.
	// Yes, this is heavy frickelei.
	auto runtime_container = store->getMetaData<pymarocco::runtime::Runtime>("marocco_runtime");

	Wafer wafer;
	boost::shared_ptr<sthal::Wafer> hardware;
	boost::shared_ptr<results::Marocco> results;

	if (!runtime_container) {
		if (wafers.empty()) {
			LOG4CXX_INFO(logger, "Could not deduce wafer, will use " << mi->default_wafer << ".");
			wafers.insert(mi->default_wafer);
		}

		wafer = *wafers.begin();
		hardware = boost::make_shared<sthal::Wafer>(wafer);
		results = boost::make_shared<results::Marocco>();
	} else if (!runtime_container->wafer() || !runtime_container->results()) {
		throw std::runtime_error("passed-in data structures are invalid");
	} else {
		hardware = runtime_container->wafer();
		results = runtime_container->results();
		wafer = hardware->index();

		if (!wafers.empty() && wafer != *wafers.begin()) {
			throw std::runtime_error(
			    "wafer provided via marocco_runtime object has wrong coordinate");
		}

		// drop all results if mapping will run
		if (!mi->skip_mapping) {
			runtime_container->clear_results();
		}
	}

	//  ——— LOAD DEFECT DATA ———————————————————————————————————————————————————

	resource_manager_t resources{load_redman_backend(mi->defects)};

	{
		auto res = redman::resources::WaferWithBackend(resources.backend(), wafer);
		auto const hicanns = res.hicanns();

		for (auto const& pair : mi->defects.hicanns()) {
			if (!pair.second) {
				LOG4CXX_DEBUG(log4cxx::Logger::getLogger("marocco"),
				              "Marked " << pair.first << " as defect/disabled");
				hicanns->disable(pair.first.toHICANNOnWafer());
			} else {
				res.inject(pair.first.toHICANNOnWafer(), pair.second);
			}
		}

		resources.inject(res);
	}

	//  ——— RUN MAPPING ————————————————————————————————————————————————————————

	Mapper mapper{*hardware, resources, mi, results};

	if (mi->skip_mapping) {
		if (!runtime_container) {
			if (mi->persist.empty()) {
				throw std::runtime_error("persist must be set for skip_mapping option to work");
			}
			LOG4CXX_INFO(logger, "Loading mapping results from " << mi->persist);
			mapper.results()->load(mi->persist.c_str());
			if (mi->wafer_cfg.empty()) {
				throw std::runtime_error("wafer_cfg must be set for skip_mapping option to work");
			}
			LOG4CXX_INFO(logger, "Loading hardware configuration from " << mi->wafer_cfg);
			hardware->load(mi->wafer_cfg.c_str());
		}
		LOG4CXX_INFO(logger, "Mapping will be skipped");
	}

	// Even when skip_mapping is true we need to setup the bio graph.
	// (Alternatively the bio graph could be persisted to the mapping results object, too).
	mapper.run(*store);
	mi->setStats(mapper.getStats());

	//  ——— CONFIGURE HARDWARE —————————————————————————————————————————————————

	static const double ms_to_s = 1e-3;

	experiment::parameters::Experiment exp_params = mi->experiment;
	if (exp_params.bio_duration_in_s() != 0.) {
		LOG4CXX_WARN(
			logger, "Discarded experiment duration set via mapping parameters "
			"in favor of duration specified via argument to pynn.run()");
	}
	exp_params.bio_duration_in_s(store->getDuration() * ms_to_s);

	results = mapper.results();

	// Configure analog outputs.
	{
		experiment::AnalogOutputsConfigurator analog_outputs(results->analog_outputs);
		analog_outputs.configure(*hardware);
	}

	// Configure external spike input.
	{
		hardware->clearSpikes();
		experiment::SpikeTimesConfigurator spike_times_configurator(
		    results->placement, results->spike_times, exp_params);
		for (auto const& hicann : hardware->getAllocatedHicannCoordinates()) {
			spike_times_configurator.configure(*hardware, hicann);
		}
	}

	if (!runtime_container) {
		// Dump sthal configuration container.
		if (!mi->wafer_cfg.empty()) {
			hardware->dump(mi->wafer_cfg.c_str(), /*overwrite=*/true);
		}

		if (!mi->persist.empty()) {
			LOG4CXX_INFO(logger, "Saving results to " << mi->persist);
			results->save(mi->persist.c_str(), true);
		}
	}

	MappingResult result;
	result.error = 0;
	result.store = store;

	//  ——— RUN EXPERIMENT —————————————————————————————————————————————————————

	if (mi->backend == PyMarocco::Backend::None) {
		return result;
	}

	DeleteRecursivelyOnScopeExit cleanup;
	std::unique_ptr<sthal::ExperimentRunner> runner;
	std::unique_ptr<sthal::HardwareDatabase> hwdb;
	std::unique_ptr<sthal::HICANNConfigurator> configurator;

	switch (mi->backend) {
		case PyMarocco::Backend::ESS: {
#ifdef HAVE_ESS
			LOG4CXX_INFO(
				logger, "Backend: ESS " << exp_params.hardware_duration_in_s() << "s");
			std::string ess_dir = mi->ess_temp_directory;
			if (ess_dir.empty()) {
				ess_dir = create_temporary_directory("ess_%%%%-%%%%-%%%%-%%%%");
				cleanup.path = ess_dir;
			}
			hwdb.reset(new sthal::ESSHardwareDatabase(ess_dir));
			runner.reset(new sthal::ESSRunner(exp_params.hardware_duration_in_s(), mi->ess_config));
#else  // HAVE_ESS
			throw std::runtime_error("ESS not available (compile with ESS)");
#endif // HAVE_ESS
			break;
		}
		case PyMarocco::Backend::Hardware: {
			LOG4CXX_INFO(
				logger, "Backend: Hardware " << exp_params.hardware_duration_in_s() << "s");
			hwdb.reset(new sthal::MagicHardwareDatabase());
			runner.reset(new sthal::ExperimentRunner(
				exp_params.hardware_duration_in_s(),
				exp_params.discard_background_events()));
			break;
		}
		default:
			throw std::runtime_error("unknown backend");
	}

	switch(mi->hicann_configurator) {
		case PyMarocco::HICANNCfg::HICANNConfigurator:
			configurator.reset(new sthal::HICANNConfigurator());
			break;
		case PyMarocco::HICANNCfg::HICANNv4Configurator:
			configurator.reset(new sthal::HICANNv4Configurator());
			break;
		case PyMarocco::HICANNCfg::DontProgramFloatingGatesHICANNConfigurator:
			configurator.reset(new sthal::DontProgramFloatingGatesHICANNConfigurator());
			break;
		case PyMarocco::HICANNCfg::NoFGConfigurator:
			configurator.reset(new sthal::NoFGConfigurator());
			break;
		case PyMarocco::HICANNCfg::NoResetNoFGConfigurator:
			configurator.reset(new sthal::NoResetNoFGConfigurator());
			break;
		case PyMarocco::HICANNCfg::OnlyNeuronNoResetNoFGConfigurator:
			configurator.reset(new sthal::OnlyNeuronNoResetNoFGConfigurator());
			break;
		case PyMarocco::HICANNCfg::ParallelHICANNv4Configurator:
			configurator.reset(new sthal::ParallelHICANNv4Configurator());
			break;
		case PyMarocco::HICANNCfg::ParallelHICANNNoFGConfigurator:
			configurator.reset(new sthal::ParallelHICANNNoFGConfigurator());
			break;
		case PyMarocco::HICANNCfg::ParallelHICANNNoResetNoFGConfigurator:
			configurator.reset(new sthal::ParallelHICANNNoResetNoFGConfigurator());
			break;
		default:
			throw std::runtime_error("unknown configurator");
	}

	experiment::Experiment experiment(
		*hardware, *results, mapper.bio_graph(), exp_params, *mi, *runner);

	hardware->commonFPGASettings()->setPLL(mi->pll_freq);

	hardware->connect(*hwdb);
	hardware->configure(*configurator);

	if(mi->backend == PyMarocco::Backend::Hardware) {
		auto verify_configurator = sthal::VerifyConfigurator();

		switch (mi->verification) {
		case PyMarocco::Verification::Skip:
			// do nothing
			break;
		case PyMarocco::Verification::VerifyButIgnore: {
			hardware->configure(verify_configurator);
			LOG4CXX_WARN(logger, verify_configurator);
			break;
		}
		case PyMarocco::Verification::Verify: {
			hardware->configure(verify_configurator);
			LOG4CXX_ERROR(logger, verify_configurator);
			if(verify_configurator.error_count() > 0) {
				throw std::runtime_error("verification of configuration failed");
			}
			break;
		}
		default:
			throw std::runtime_error("unknown verification mode");
		}
	}

	experiment.run();

	//  ——— EXTRACT RESULTS ————————————————————————————————————————————————————

	experiment.extract_results(*store);

	LOG4CXX_INFO(logger, "Finished");
	return result;
}

} // namespace mapping
} // namespace marocco
