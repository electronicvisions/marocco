#include <tuple>
#include <stdexcept>
#include <tuple>
#include <utility>

#include "marocco/resource/Manager.h"

#include "redman/resources/Wafer.h"
#include "marocco/Logger.h"
#include "marocco/resource/CalibParameters.h"
#include "pymarocco/PyMarocco.h"

#include "marocco/routing/L1BusGlobal.h"
#include "marocco/routing/L1BusOnWafer.h"

#include "calibtic/HMF/HICANNCollection.h"

#include "hal/Coordinate/iter_all.h"

namespace calibtic {
namespace backend {
	class XMLBackend;
	class BinaryBackend;
}
}

namespace marocco {
namespace resource {

template <typename T>
boost::shared_ptr<const typename RedmanManagerType<T>::type> get_components(
    redman::resources::WaferWithBackend const& manager);

template <>
boost::shared_ptr<const typename RedmanManagerType<HMF::Coordinate::HICANNGlobal>::type>
get_components<HMF::Coordinate::HICANNGlobal>(redman::resources::WaferWithBackend const& manager)
{
	return manager.hicanns();
}

template <>
boost::shared_ptr<const typename RedmanManagerType<HMF::Coordinate::FPGAGlobal>::type>
get_components<HMF::Coordinate::FPGAGlobal>(redman::resources::WaferWithBackend const& manager)
{
	return manager.fpgas();
}

template <class T>
Manager<T>::Manager(
    boost::shared_ptr<redman::backend::Backend> backend,
    std::set<wafer_type> const& wafers,
    boost::optional<pymarocco::PyMarocco> const pymarocco)
    : mBackend(backend), m_pymarocco(pymarocco)
{
	for (auto const& wafer : wafers) {
		load(wafer);
	}
}

template <class T>
void Manager<T>::reload() {
	std::set<wafer_type> wafers;

	for (auto const& pair : mWafers) {
		wafers.insert(pair.first);
	}

	mWafers.clear();

	for (auto const& wafer : wafers) {
		load(wafer);
	}
}

template <class T>
void Manager<T>::load(wafer_type const& wafer) {
	auto it = mWafers.find(wafer);

	if (it != mWafers.end())
		throw std::runtime_error("Wafer has already been loaded.");

	mWafers.insert(
           std::make_pair(wafer, manager_type(mBackend, wafer)));
}

template <class T>
void Manager<T>::inject(manager_type const& wafer) {
	auto pair = mWafers.insert(std::make_pair(wafer.id(), wafer));
	if (!pair.second)
		throw std::runtime_error("Wafer has already been loaded.");
}

template <class T>
auto Manager<T>::wafers() const -> std::vector<wafer_type> {
	std::vector<wafer_type> ws;
	ws.reserve(mWafers.size());
	for (auto const& pair : mWafers) {
		ws.push_back(pair.first);
	}
	return ws;
}

template <class T>
bool Manager<T>::has(resource_type const& r) const {
	return (wafer_for(r) != mWafers.end()
	        && mMasked.find(r) == mMasked.end());
}

template <class T>
auto Manager<T>::wafer_for(resource_type const& r) const
	-> wafer_map_type::const_iterator {
	typename resource_type::mixed_in_type wafer;
	typename resource_type::local_type local;
	std::tie(wafer, local) = r.split();

	auto it = mWafers.find(wafer);

	if (it != mWafers.end() && it->second.has(local))
		return it;

	return mWafers.end();
}

template <class T>
boost::shared_ptr<const typename Manager<T>::redman_resources_type>
Manager<T>::get(resource_type const& r) const {
	if (!has(r))
		throw std::runtime_error("Resource not present.");

	typename resource_type::mixed_in_type wafer;
	typename resource_type::local_type local;
	std::tie(wafer, local) = r.split();

	auto it = wafer_for(r);
	return it->second.get(local);
}

template <class T>
void Manager<T>::mask(resource_type const& r) {
	if (wafer_for(r) == mWafers.end())
		throw std::runtime_error("Resource not present.");

	auto it = mMasked.insert(r);
	if (!it.second)
		throw std::runtime_error("Resource has already been masked.");
}

template <class T>
void Manager<T>::unmask(resource_type const& r) {
	if (wafer_for(r) == mWafers.end())
		throw std::runtime_error("Resource not present.");

	auto it = mMasked.find(r);
	if (it == mMasked.end())
		throw std::runtime_error("Resource has not been masked yet.");

	mMasked.erase(it);
}

template <class T>
bool Manager<T>::masked(resource_type const& r) const {
	return mMasked.find(r) != mMasked.end();
}

template <class T>
void Manager<T>::allocate(resource_type const& r) {
	if (!has(r))
		throw std::runtime_error("Resource not present.");

	auto it = mAllocated.insert(r);
	if (!it.second)
		throw std::runtime_error("Resource has already been allocated.");
}

template <class T>
void Manager<T>::release(resource_type const& r) {
	if (!has(r))
		throw std::runtime_error("Resource not present.");

	auto it = mAllocated.find(r);
	if (it == mAllocated.end())
		throw std::runtime_error("Resource has not been allocated yet.");

	mAllocated.erase(it);
}

template <class T>
bool Manager<T>::available(resource_type const& r) const {
	return (has(r) && mAllocated.find(r) == mAllocated.end());
}

template <class T>
size_t Manager<T>::count_present() const {
	size_t available = 0;
	for (auto const& pair : mWafers) {
		available += get_components<T>(pair.second)->available();
	}
	return available - mMasked.size();
}

template <class T>
size_t Manager<T>::count_available() const {
	return count_present() - mAllocated.size();
}

template <class T>
size_t Manager<T>::count_allocated() const {
	return mAllocated.size();
}

template <class T>
auto Manager<T>::begin(typename iterator_type::mode_type mode) const -> iterator_type {
	typename iterator_type::queue_type queue;

	for (auto it = mWafers.cbegin(); it != mWafers.cend(); ++it) {
		queue.push(it);
	}

	return iterator_type{mMasked, mAllocated, std::move(queue), mode};
}

template <class T>
auto Manager<T>::end(typename iterator_type::mode_type mode) const -> iterator_type {
	typename iterator_type::queue_type queue;

	return iterator_type{mMasked, mAllocated, std::move(queue), mode};
}

template <class T>
Manager<T>::iterator_type::iterator_type(
	allocation_type const& mask,
	allocation_type const& alloc,
	queue_type&& q,
	mode_type mode)
    : mMode(mode),
      mMasked(mask),
      mAllocated(alloc),
      mWafer(),
      mCurrent(),
      mIter(),
      mQueue(q) {
	pop();
	check_maybe_increment();
}

template <class T>
bool Manager<T>::iterator_type::equal(iterator_type const& other) const {
	return mMode == other.mMode
		&& mWafer == other.mWafer
		&& mCurrent == other.mCurrent
		&& mIter == other.mIter
		&& mQueue == other.mQueue;
}

template <class T>
void Manager<T>::iterator_type::increment() {
	++mIter;

	if (mIter == mCurrent->end())
		pop();

	check_maybe_increment();
}

template <class T>
void Manager<T>::iterator_type::check_maybe_increment() {
	while (mCurrent && mIter != mCurrent->end()) {
		auto resource = dereference();
		auto masked = (mMasked.find(resource) != mMasked.end());
		auto allocated = (mAllocated.find(resource) != mAllocated.end());

		switch (mMode) {
			case PRESENT:
				if (!masked)
					return;
				break;
			case AVAILABLE:
				if (!masked && !allocated)
					return;
				break;
			case ALLOCATED:
				if (!masked && allocated)
					return;
				break;
		}

		++mIter;

		if (mIter == mCurrent->end())
			pop();
	}
}

template <class T>
auto Manager<T>::iterator_type::dereference() const -> resource_type {
	// if (!mCurrent)
	// 	throw std::runtime_error("Dereferencing end iterator.");
	return resource_type(*mIter, mWafer);
}


template <class T>
void Manager<T>::iterator_type::pop() {
	if (mQueue.empty()) {
		mWafer = HMF::Coordinate::Wafer{};
		mCurrent.reset();
		mIter = underlying_iterator_type{};
		return;
	}

	auto pair = *mQueue.front();
	mWafer = pair.first;
	mCurrent = get_components<T>(pair.second);
	mIter = mCurrent->begin();
	mQueue.pop();
}

template <class T>
boost::shared_ptr<HMF::HICANNCollection> Manager<T>::loadCalib(HMF::Coordinate::HICANNGlobal const& hicann_global) const
{
	// if calib is in storage return that
	if (mCalibs.find(hicann_global) == mCalibs.end()) {
		//  ——— LOAD HARDWARE CONSTRAINTS FROM CALIBRATION —————————————
		if (m_pymarocco) {
			boost::shared_ptr<calibtic::backend::Backend> calib_backend;
			switch (m_pymarocco->calib_backend) {
				case pymarocco::PyMarocco::CalibBackend::XML:
					calib_backend =
					    BackendLoaderCalib::load_calibtic_backend<calibtic::backend::XMLBackend>(
					        m_pymarocco->calib_path);
					break;
				case pymarocco::PyMarocco::CalibBackend::Binary:
					calib_backend =
					    BackendLoaderCalib::load_calibtic_backend<calibtic::backend::BinaryBackend>(
					        m_pymarocco->calib_path);
					break;
				case pymarocco::PyMarocco::CalibBackend::Default:
					MAROCCO_WARN("using default Backend");
					break;
				default:
					throw std::runtime_error("unknown calibration backend type");
			}
			CalibParameters calib_parameters(hicann_global, *m_pymarocco, calib_backend);
			auto calib = calib_parameters.getCalibrationData(/*fallback to default*/ true);
			mCalibs[hicann_global] = calib;
		} else {
			MAROCCO_WARN("failed to load Calib data for: " << hicann_global);
		}
	}

	return mCalibs.at(hicann_global);
}

template <class T>
size_t Manager<T>::getMaxL1Crossbars(marocco::routing::L1BusOnWafer const& bus) const
{
	size_t max_switches = 0;
	// load data from "cache" if present. load from calib if needed.
	// TODO deduce correct wafer from somewhere
	for (auto const wafer : mWafers) {
		marocco::routing::L1BusGlobal bus_g{bus, wafer.first};
		// load calib data for this bus_g
		auto const hicann_on_wafer = bus_g.toL1BusOnWafer().toHICANNOnWafer();
		auto const hicann_global = HMF::Coordinate::HICANNGlobal(hicann_on_wafer, wafer.first);

		auto const calib = loadCalib(hicann_global);

		size_t switches = 0;
		if (bus.is_vertical()) {
			auto line = bus.toVLineOnHICANN();
			switches = std::min(
			    calib->atL1CrossbarCollection()->getMaxSwitchesPerRow(line),
			    calib->atL1CrossbarCollection()->getMaxSwitchesPerColumn(line));

		} else {
			auto line = bus.toHLineOnHICANN();
			switches = std::min(
			    calib->atL1CrossbarCollection()->getMaxSwitchesPerRow(line),
			    calib->atL1CrossbarCollection()->getMaxSwitchesPerColumn(line));
		}

		if (max_switches == 0 || switches < max_switches) {
			max_switches = switches;
		}
	}

	return max_switches;
}

template <class T>
size_t Manager<T>::getMaxChainLength(HMF::Coordinate::HICANNOnWafer const& hicann) const
{
	size_t max_chainLength = 0;
	for( auto const bus : HMF::Coordinate::iter_all<HMF::Coordinate::VLineOnHICANN>() ){
		size_t const length = getMaxChainLength( marocco::routing::L1BusOnWafer(hicann, bus) );
		if ( max_chainLength == 0 || length < max_chainLength){
			max_chainLength = length;
		}
	}
	return max_chainLength;
}

template <class T>
size_t Manager<T>::getMaxChainLength(marocco::routing::L1BusOnWafer const& bus) const
{
	size_t max_chainLength = 0;

	for (auto const wafer : mWafers) {
		marocco::routing::L1BusGlobal bus_g{bus, wafer.first};

		auto const hicann_on_wafer = bus_g.toL1BusOnWafer().toHICANNOnWafer();
		auto const hicann_global = HMF::Coordinate::HICANNGlobal(hicann_on_wafer, wafer.first);

		auto const calib = loadCalib(hicann_global);

		size_t chainlength = calib->atSynapseChainLengthCollection()->getMaxChainLength(bus.toVLineOnHICANN());
		if (max_chainLength == 0 || chainlength < max_chainLength) {
			max_chainLength = chainlength;
		}
	}
	MAROCCO_DEBUG("max chain length: " << max_chainLength);
	return max_chainLength;
}

} // namespace resource
} // namespace marocco
