#include "marocco/mapping.h"

#include <cstdlib>
#include <memory>
#include <stdexcept>
#include <boost/filesystem.hpp>
#include <boost/make_shared.hpp>

#include "redman/resources/Wafer.h"
#include "sthal/ESSHardwareDatabase.h"
#include "sthal/ESSRunner.h"
#include "sthal/ExperimentRunner.h"
#include "sthal/HICANNConfigurator.h"
#include "sthal/MagicHardwareDatabase.h"
#include "sthal/OnlyNeuronNoResetNoFGConfigurator.h"
#include "sthal/VerifyConfigurator.h"

#include "marocco/config.h"
#include "marocco/Logger.h"
#include "marocco/Mapper.h"
#include "marocco/experiment/AnalogOutputsConfigurator.h"
#include "marocco/experiment/Experiment.h"
#include "marocco/experiment/SpikeTimesConfigurator.h"
#include "marocco/experiment/ReadRepeaterTestdata.h"
#include "marocco/resource/BackendLoaderRedman.h"
#include "pymarocco/PyMarocco.h"
#include "pymarocco/runtime/Runtime.h"

using namespace halco::hicann::v2;
using pymarocco::PyMarocco;

namespace redman {
namespace backend {
class XMLBackend;
class Without;
}
}


namespace {

#ifdef HAVE_ESS
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
#endif

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

using namespace euter;

std::set<Wafer> wafers_used_in(ObjectStore& store)
{
	auto mi = store.getMetaData<PyMarocco>("marocco");

	std::set<Wafer> wafers;

	if (mi->defects.wafer()) {
		wafers.insert(mi->defects.wafer()->id());
	}

	return wafers;
}

void run(ObjectStore& store) {
	using pymarocco::PyMarocco;

	log4cxx::LoggerPtr const logger = log4cxx::Logger::getLogger("marocco");

	auto mi = store.getMetaData<PyMarocco>("marocco");
	auto wafers = wafers_used_in(store);

	if (wafers.size() > 1) {
		throw std::runtime_error("currently only a single wafer is supported.");
	}

	// When marocco lives inside the same process as pyhmf, the user may pass in a wafer
	// config pointer to work around the overhead of dumping / loading the configuration
	// and connecting / resetting the hardware in “in the loop”-style experiments.
	// Yes, this is heavy frickelei.
	auto runtime_container = store.getMetaData<pymarocco::runtime::Runtime>("marocco_runtime");

	if (runtime_container != nullptr && (!mi->persist.empty() || !mi->wafer_cfg.empty())) {
		throw std::runtime_error("usage of runtime and persist or wafer_cfg are mutally exclusive");
	}

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
	}

	//  ——— LOAD DEFECT DATA ———————————————————————————————————————————————————

	boost::shared_ptr<redman::backend::Backend> redman_backend;
	switch (mi->defects.backend) {
		case pymarocco::Defects::Backend::XML:
			redman_backend = marocco::resource::BackendLoaderRedman::load_redman_backend<
			    redman::backend::XMLBackend>(mi->defects.path);
			break;
		case pymarocco::Defects::Backend::Without:
			redman_backend =
			    marocco::resource::BackendLoaderRedman::load_redman_backend<redman::backend::Without>(
			        mi->defects.path);
			break;
		default:
			throw std::runtime_error("unknown redman backend type");
	}
	resource_manager_t resources{redman_backend, boost::make_optional(*mi)};
	resource_fpga_manager_t fpga_resources{redman_backend, boost::make_optional(*mi)};

	// if the user did not supply a redman wafer resource, load it
	if (!mi->defects.wafer()) {
		mi->defects.set(
		    boost::make_shared<redman::resources::WaferWithBackend>(resources.backend(), wafer));
	}

	// need to set defects as early as possible
	hardware->set_defects(mi->defects.wafer());

	// Take data either from injected redman Wafer or load new from backend
	auto res = *(mi->defects.wafer());
	auto res_fpga = res;
	{
		auto const hicanns = res.hicanns();

		// HICANNs
		for (auto const& h : hicanns->disabled()) {
			LOG4CXX_TRACE(log4cxx::Logger::getLogger("marocco"), "Marked " << h << " as defect/disabled");
		}

		size_t const n_marked_hicanns = boost::size(hicanns->disabled());
		if (n_marked_hicanns != 0) {
			LOG4CXX_DEBUG(log4cxx::Logger::getLogger("marocco"),
			              "Marked " << n_marked_hicanns
			                        << " HICANN(s) as defect/disabled");
		}

		resources.inject(res);

		auto const fpgas = res_fpga.fpgas();
		// FPGAS
		for (auto const& f : fpgas->disabled()) {
			LOG4CXX_TRACE(
			    log4cxx::Logger::getLogger("marocco"), "Marked " << f << " as defect/disabled");
		}

		size_t const n_marked_fpgas = boost::size(fpgas->disabled());
		if (n_marked_fpgas != 0) {
			LOG4CXX_DEBUG(
			    log4cxx::Logger::getLogger("marocco"),
			    "Marked " << n_marked_fpgas << " FPGA(s) as defect/disabled");
		}

		fpga_resources.inject(res_fpga);
	}

	//  ——— RUN MAPPING ————————————————————————————————————————————————————————

	Mapper mapper{*hardware, resources, fpga_resources, mi, results};

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
	mapper.run(store);
	mi->setStats(mapper.getStats());

	//  ——— CONFIGURE HARDWARE —————————————————————————————————————————————————

	static const double ms_to_s = 1e-3;

	experiment::parameters::Experiment exp_params = mi->experiment;
	if (exp_params.bio_duration_in_s() != 0.) {
		LOG4CXX_WARN(
			logger, "Discarded experiment duration set via mapping parameters "
			"in favor of duration specified via argument to pynn.run()");
	}
	exp_params.bio_duration_in_s(store.getDuration() * ms_to_s);

	results = mapper.results();

	experiment::ReadRepeaterTestdata
		repeater_test(*results,
		              mi->n_locking_retest,
		              mi->n_locking_recheck_after_ok,
		              mi->time_between_rechecks);

	if (mi->checkl1locking == PyMarocco::CheckL1Locking::Check ||
		mi->checkl1locking == PyMarocco::CheckL1Locking::CheckButIgnore) {
		repeater_test.configure(*hardware);
	}

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

	//  ——— RUN EXPERIMENT —————————————————————————————————————————————————————

	if (mi->backend == PyMarocco::Backend::Without) {
		return;
	}

	DeleteRecursivelyOnScopeExit cleanup;
	std::unique_ptr<sthal::ExperimentRunner> runner;
	std::unique_ptr<sthal::HardwareDatabase> hwdb;

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
			hwdb.reset(new sthal::ESSHardwareDatabase(wafer, ess_dir));
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

	experiment::Experiment experiment(
		*hardware, *results, mapper.bio_graph(), exp_params, *mi, *runner);

	hardware->connect(*hwdb);
	boost::shared_ptr<sthal::HICANNConfigurator> configurator = mi->hicann_configurator;
	hardware->configure(*configurator);

	if(mi->backend == PyMarocco::Backend::Hardware) {

		if (mi->verification != PyMarocco::Verification::Skip) {
			std::vector<SynapseOnWafer> synapses_to_be_verified;
			for (auto const& syn : results->synapse_routing.synapses()) {
				if (syn.hardware_synapse()) {
					synapses_to_be_verified.push_back(*syn.hardware_synapse());
				}
			}
			auto synapse_policy = sthal::VerifyConfigurator::SynapsePolicy::Mask;
			auto verify_configurator = sthal::VerifyConfigurator(true, synapse_policy);
			verify_configurator.set_synapse_mask(synapses_to_be_verified);
			hardware->configure(verify_configurator);

			if (verify_configurator.error_count() == 0) {
				LOG4CXX_INFO(logger, verify_configurator);
			} else {
				switch (mi->verification) {
					case PyMarocco::Verification::Skip:
						break;
					case PyMarocco::Verification::VerifyButIgnore:
						LOG4CXX_WARN(logger, verify_configurator);
						break;
					case PyMarocco::Verification::Verify:
						LOG4CXX_ERROR(logger, verify_configurator);
						throw std::runtime_error("verification of configuration failed");
						break;
					default:
						throw std::runtime_error("unknown verification mode");
				}
			}
		}

		std::map<HICANNOnWafer, sthal::Neurons> original_neuron_configuration;
		auto only_neuron_no_reset_configurator = sthal::OnlyNeuronNoResetNoFGConfigurator();
		if (mi->checkl1locking == PyMarocco::CheckL1Locking::Check ||
		    mi->checkl1locking == PyMarocco::CheckL1Locking::CheckButIgnore) {
			// disable L1 output of all neurons
			for (auto hicann : hardware->getAllocatedHicannCoordinates()) {
				original_neuron_configuration[hicann] = (*hardware)[hicann].neurons;
				(*hardware)[hicann].disable_l1_output();
			}

			hardware->configure(only_neuron_no_reset_configurator);
		}

		switch (mi->checkl1locking) {
			case PyMarocco::CheckL1Locking::SkipCheck:
				// do nothing
				break;
			case PyMarocco::CheckL1Locking::Check:
			case PyMarocco::CheckL1Locking::CheckButIgnore: {
				auto const locked = repeater_test.check(*hardware, ::HMF::HICANN::BkgRegularISI(mi->bkg_gen_isi));
				if (mi->checkl1locking == PyMarocco::CheckL1Locking::Check && !locked) {
					LOG4CXX_ERROR(logger, "Aborting because L1 locking failed");
					throw std::runtime_error("L1 locking failed");
				}

				// reenable L1 output of all neurons
				for (auto hicann : hardware->getAllocatedHicannCoordinates()) {
					(*hardware)[hicann].neurons = original_neuron_configuration[hicann];
				}

				hardware->configure(only_neuron_no_reset_configurator);
				break;
			}
			default:
				throw std::runtime_error("unknown l1 locking check mode");
		}
	}

	experiment.run();

	//  ——— EXTRACT RESULTS ————————————————————————————————————————————————————

	experiment.extract_results(store);

	LOG4CXX_INFO(logger, "Finished");
	return;
}

} // namespace mapping
} // namespace marocco
