#include "marocco/mapping.h"
#include "marocco/Logger.h"

#include <stdexcept>
#include <boost/make_shared.hpp>

#include "redman/backend/MockBackend.h"
#include "redman/resources/Wafer.h"

namespace marocco {
namespace mapping {

std::set<HMF::Coordinate::Wafer> wafers_used_in(boost::shared_ptr<ObjectStore> store) {
	auto mi = store->getMetaData<pymarocco::PyMarocco>("marocco");

	std::set<HMF::Coordinate::Wafer> wafers;

	for (auto const& defect : mi->defects.hicanns()) {
		wafers.insert(defect.first.toWafer());
	}

	for (auto const& placement : mi->placement) {
		for (auto const& hicann : placement.second.first) {
			wafers.insert(hicann.toWafer());
		}
	}

	return wafers;
}

MappingResult run(boost::shared_ptr<ObjectStore> store,
                  Mapper::comm_type const& comm) {
	using pymarocco::PyMarocco;
	auto mi = store->getMetaData<PyMarocco>("marocco");

	auto wafers = wafers_used_in(store);

	if (wafers.empty()) {
		LOG4CXX_INFO(log4cxx::Logger::getLogger("marocco"),
		             "Could not deduce wafer, will use "
		             << mi->default_wafer << ".");
		wafers.insert(mi->default_wafer);
	} else if (wafers.size() > 1) {
		throw std::runtime_error("Currently only a single wafer is supported.");
	}

	auto const wafer = *wafers.begin();

	// FIXME: use resource manager with proper backend
	resource_manager_t resources{boost::make_shared<redman::backend::MockBackend>()};

	{
		auto res = redman::resources::WaferWithBackend(resources.backend(), wafer);

		for (auto const& pair : mi->defects.hicanns()) {
			res.inject(pair.first.toHICANNOnWafer(), pair.second);
		}

		resources.inject(res);
	}

	hardware_system_t hw{};
	hw[wafer]; // FIXME: hack to allocate one wafer

	Mapper mapper{hw, resources, comm, mi};

	// only map if no wafer cfg is injected
	if(mi->wafer_cfg_inject.empty()) {
		mapper.run(*store);
	} else {
		// load wafer from file
		LOG4CXX_INFO(log4cxx::Logger::getLogger("marocco"),
					 "injecting wafer configuration from: \"" <<
					 mi->wafer_cfg_inject << "\"");
		hw[wafer].load(mi->wafer_cfg_inject.c_str());
	}

	// pyoneer doesn't work currently
	// auto pyoneer = objectstore->getMetaData<HMF::PyOneer>("pyoneer");

	// C O N F I G U R E   H A R D W A R E
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

		experiment::ReadResults reader{*mi, hw, resources};
		reader.run(*store, *mapper.getLookupTable());
	}

	mi->setStats(mapper.getStats());

	MappingResult result{};

	result.error = 0;
	result.store = store;

	return result;
}

} // namespace mapping
} // namespace marocco
