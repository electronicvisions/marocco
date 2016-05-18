#include "marocco/mapping.h"
#include "marocco/Logger.h"

#include <cstdlib>
#include <stdexcept>
#include <boost/make_shared.hpp>

#include "redman/backend/Backend.h"
#include "redman/backend/Library.h"
#include "redman/backend/MockBackend.h"
#include "redman/resources/Wafer.h"

#include "control/Control.h"
#include "marocco/experiment/AnalogOutputsConfigurator.h"
#include "marocco/experiment/ReadResults.h"

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

	// pyoneer doesn't work currently
	// auto pyoneer = objectstore->getMetaData<HMF::PyOneer>("pyoneer");

	// C O N F I G U R E   H A R D W A R E

	auto const& results = mapper.results();

	// Configure analog outputs.
	{
		experiment::AnalogOutputsConfigurator analog_outputs(results.analog_outputs);
		analog_outputs.configure(hw[wafer]);
	}

	// backend None is handled gracefully within Control
	control::Control control{hw, resources, *mi};

	// calculate duration of hw experiment
	const double duration_in_ms = store->getDuration();
	static const double ms_to_s = 1e-3;
	const double hw_duration =
	    duration_in_ms * ms_to_s / mi->speedup + mi->experiment_time_offset;
	control.run(hw_duration);

	PyMarocco::Backend backend = mi->backend;
	if (backend != PyMarocco::Backend::None) {
		// HMF::Handle::get_pyoneer().startESS(1000);

		// R E A D   R E S U L T S   B A C K
		// TODO: implement runtime data object

		// R E V E R S E   M A P P I N G
		// TODO: translate hw data back to bio data
		// TODO: merge results and send it back to user

		experiment::ReadResults reader(
			*mi, hw, mapper.results().placement, mapper.bio_graph(), wafer);
		reader.run(*store);
	}

	mi->setStats(mapper.getStats());

	MappingResult result{};

	result.error = 0;
	result.store = store;

	return result;
}

} // namespace mapping
} // namespace marocco
