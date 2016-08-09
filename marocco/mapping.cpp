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
#include "sthal/MagicHardwareDatabase.h"

#include "marocco/Logger.h"
#include "marocco/Mapper.h"
#include "marocco/experiment/AnalogOutputsConfigurator.h"
#include "marocco/experiment/Experiment.h"
#include "marocco/experiment/SpikeTimesConfigurator.h"

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

	if (wafers.empty()) {
		LOG4CXX_INFO(logger, "Could not deduce wafer, will use " << mi->default_wafer << ".");
		wafers.insert(mi->default_wafer);
	} else if (wafers.size() > 1) {
		throw std::runtime_error("Currently only a single wafer is supported.");
	}

	auto const wafer = *wafers.begin();

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

	hardware_system_t hw{};
	hw[wafer]; // FIXME: hack to allocate one wafer

	//  ——— RUN MAPPING ————————————————————————————————————————————————————————

	Mapper mapper{hw, resources, mi};

	if (mi->skip_mapping) {
		if (mi->wafer_cfg.empty() || mi->persist.empty()) {
			throw std::runtime_error(
			    "wafer_cfg and persist must be set for skip_mapping option to work");
		}
		LOG4CXX_INFO(logger, "Mapping will be skipped");
		LOG4CXX_INFO(logger, "Loading wafer configuration from " << mi->wafer_cfg);
		hw[wafer].load(mi->wafer_cfg.c_str());
		LOG4CXX_INFO(logger, "Loading mapping results from " << mi->persist);
		mapper.results().load(mi->persist.c_str());
	}

	// Even when skip_mapping is true we need to setup the bio graph.
	// (Alternatively the bio graph could be persisted to the mapping results object, too).
	mapper.run(*store);
	mi->setStats(mapper.getStats());

	//  ——— CONFIGURE HARDWARE —————————————————————————————————————————————————

	static const double ms_to_s = 1e-3;

	experiment::parameters::Experiment exp_params;
	exp_params.speedup(mi->speedup);
	exp_params.offset_in_s(mi->experiment_time_offset);
	exp_params.bio_duration_in_s(store->getDuration() * ms_to_s);

	auto const& results = mapper.results();

	// Configure analog outputs.
	{
		experiment::AnalogOutputsConfigurator analog_outputs(results.analog_outputs);
		analog_outputs.configure(hw[wafer]);
	}

	// Configure external spike input.
	{
		hw[wafer].clearSpikes();
		experiment::SpikeTimesConfigurator spike_times_configurator(
		    results.placement, results.spike_times, exp_params);
		for (auto const& hicann : hw[wafer].getAllocatedHicannCoordinates()) {
			spike_times_configurator.configure(hw[wafer], hicann);
		}
	}

	// Dump sthal configuration container.
	if (!mi->wafer_cfg.empty()) {
		hw[wafer].dump(mi->wafer_cfg.c_str(), /*overwrite=*/true);
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
			runner.reset(new sthal::ESSRunner(exp_params.hardware_duration_in_s()));
#else  // HAVE_ESS
			throw std::runtime_error("ESS not available (compile with ESS)");
#endif // HAVE_ESS
			break;
		}
		case PyMarocco::Backend::Hardware: {
			LOG4CXX_INFO(
				logger, "Backend: Hardware " << exp_params.hardware_duration_in_s() << "s");
			hwdb.reset(new sthal::MagicHardwareDatabase());
			runner.reset(new sthal::ExperimentRunner(exp_params.hardware_duration_in_s()));
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
		default:
			throw std::runtime_error("unknown configurator");
	}

	experiment::Experiment experiment(
		hw[wafer], mapper.results(), mapper.bio_graph(), exp_params, *mi, *runner, *hwdb,
		*configurator);

	hw[wafer].commonFPGASettings()->setPLL(mi->pll_freq);
	experiment.run();

	//  ——— EXTRACT RESULTS ————————————————————————————————————————————————————

	experiment.extract_results(*store);

	return result;
}

} // namespace mapping
} // namespace marocco
