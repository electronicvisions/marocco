#pragma once

#include <queue>
#include <map>
#include <set>
#include <unordered_set>
#include <unordered_map>

#include <boost/iterator/iterator_facade.hpp>
#include <boost/optional.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include "halco/hicann/v2/fwd.h"
#include "redman/resources/Wafer.h"
#include "marocco/resource/BackendLoaderCalib.h"
#include "marocco/util/iterable.h"
#include "pymarocco/PyMarocco.h"

namespace redman {
class Hicann;
namespace backend {
class Backend;
} // backend
} // redman

namespace HMF {
class HICANNCollection;
} // HMF

namespace marocco {
namespace routing {
class L1BusOnWafer;
} // routing
} // marocco


using pymarocco::PyMarocco;

namespace marocco {
namespace resource {

template <typename T> struct RedmanResourcesType;

template <>
struct RedmanResourcesType<halco::hicann::v2::HICANNGlobal>
{
	typedef redman::resources::Hicann type;
};

template <>
struct RedmanResourcesType<halco::hicann::v2::FPGAGlobal>
{
	typedef redman::resources::Fpga type;
};

template <typename T> struct RedmanManagerType;

template <>
struct RedmanManagerType<halco::hicann::v2::HICANNGlobal>
{
	typedef redman::resources::components::Hicanns type;
};

template <>
struct RedmanManagerType<halco::hicann::v2::FPGAGlobal>
{
	typedef redman::resources::components::Fpgas type;
};

/** Overlay that can be used to track allocated (HICANN/FPGA/...)Globals at runtime. */
template <class T>
class Manager
{
public:
	typedef T resource_type;

private:
	typedef halco::hicann::v2::Wafer wafer_type;
	typedef redman::resources::Wafer manager_type;
	typedef std::map<wafer_type, const manager_type> wafer_map_type;
	typedef std::unordered_set<resource_type> allocation_type;

	typedef typename RedmanResourcesType<T>::type redman_resources_type;
	typedef typename RedmanManagerType<T>::type redman_manager_type;

public:
	/**
	 * Constructor for Resource Managers
	 *
	 * @param [in] backend: redman backend to handle wafers
	 * @param [in] wafers: optional, a set of wafers to handle
	 * @param [in] pymarocco: optional, holds the backend to load some data from
	 */
	Manager(
	    boost::shared_ptr<redman::backend::Backend> backend,
	    boost::optional<PyMarocco> const pymarocco)
	    : Manager(backend, {}, pymarocco){};

	Manager(
	    boost::shared_ptr<redman::backend::Backend> backend, std::set<wafer_type> const& wafers)
	    : Manager(backend, wafers, boost::none){};

	Manager(
	    boost::shared_ptr<redman::backend::Backend> backend,
	    std::set<wafer_type> const& wafers,
	    boost::optional<PyMarocco> const pymarocco);


	/** Load data for the given wafer.
	 * \throws std::runtime_error When wafer has already been loaded.
	 */
	void load(wafer_type const& wafer);

	/** Adds the given wafer resource.
	 * \throws std::runtime_error When wafer has already been loaded.
	 */
	void inject(manager_type const& wafer);

	/** Return all wafers for which data has been loaded.
	 */
	std::vector<wafer_type> wafers() const;

	/** Check whether this Resource is present.
	 * This is the case if the given Resource is managed by this instance
	 * (i.e. its wafer has been loaded) and has not been disabled in
	 * the resource management.
	 * In addition to the data provided by the resource management a
	 * Resource can be marked as absent using `mask()`.
	 */
	bool has(resource_type const& resource) const;

	/** Mark the given Resource as being absent, e.g. due to use in a
	 * different process.
	 * \throws std::runtime_error If Resource is disabled in the resource
	 *         management or when trying to mark an already masked Resource.
	 */
	void mask(resource_type const& resource);

	/** Revert the operation done by `mask()`.
	 * Its presence (see `has()`) is then only determined by the
	 * underlying defect data.
	 * \throws std::runtime_error If Resource is disabled in the resource
	 *         management or if this Resource is not masked.
	 */
	void unmask(resource_type const& resource);

	/** Check whether the given Resource has been masked (see `mask()`).
	 */
	bool masked(resource_type const& resource) const;

	/** Load the resource manager for the given Resource.
	 * \throws std::runtime_error If `has()` fails for this Resource.
	 */
	boost::shared_ptr<const redman_resources_type> get(resource_type const& resource) const;

	/** Mark the given Resource as being 'in use' (not available anymore).
	 * \throws std::runtime_error If `has()` fails for this Resource
	 *         or if trying to allocate twice.
	 */
	void allocate(resource_type const& resource);

	/** Release Resource into pool of available Resources.
	 * \throws std::runtime_error If `has()` fails for this Resource or if it
	 *         has not been allocated yet.
	 */
	void release(resource_type const& resource);

	/** Check whether the given Resource is available.
	 * In addition to `has()` this checks whether the given Resource has already
	 * been allocated.
	 */
	bool available(resource_type const& resource) const;

	/// Return the number of available Resources.
	size_t count_available() const;

	/// Return the number of present Resources.
	size_t count_present() const;

	/// Return the number of allocated Resources.
	size_t count_allocated() const;

	boost::shared_ptr<redman::backend::Backend> backend() const { return mBackend; }

	/**
	 * Returns the shared pointer to the HICANNCollection.
	 * if it is _not_ loaded yet it is stored in a map.
	 * if it is already loaded: it is loaded from that map.
	 * thus it loads only when nescessary.
	 */
	boost::shared_ptr<HMF::HICANNCollection> loadCalib(halco::hicann::v2::HICANNGlobal const& hicann_global) const;

	/**
	 * Returns the maximum number of crossbars, that are allowed to be used on this bus
	 */
	size_t getMaxL1Crossbars(marocco::routing::L1BusOnWafer const& bus) const;

	/**
	 * Retuns the maximum Chain Length, that is allowed on this bus
	 */
	size_t getMaxChainLength(marocco::routing::L1BusOnWafer const& bus) const;

	/**
	 * Returns the minimal maximum ChainLength that all VBuses on this HICANN allow.
	 *
	 * if the maxima are 3,4,5 it returns 3.
	 */
	size_t getMaxChainLength(halco::hicann::v2::HICANNOnWafer const& hicann) const;

	/**
	 * Returns the maximum number of Switches from a specific bus into the synapse array.
	 */
	size_t getMaxSynapseSwitches(marocco::routing::L1BusOnWafer const& bus) const;

protected:
	friend class AHICANNManager;
	friend class AFPGAManager;

	/** Reload all data from the backend (for use in tests).
	 * Does not touch the set of allocated Resources.  Note that wafers
	 * added via `inject()` will also be reloaded from the backend.
	 */
	void reload();

private:
	/** Returns an iterator to the wafer map entry belonging to the given
	 * Resource if the checks outlined in the description of `has()` succeed.
	 * Else a past-the-end iterator is returned.
	 */
	wafer_map_type::const_iterator wafer_for(resource_type const& resource) const;

	boost::shared_ptr<redman::backend::Backend> mBackend;
	boost::optional<PyMarocco> const m_pymarocco;
	wafer_map_type mWafers;
	allocation_type mMasked;
	allocation_type mAllocated;

	// Cache for Calibration data
	mutable std::unordered_map<halco::hicann::v2::HICANNGlobal, boost::shared_ptr<HMF::HICANNCollection> > mCalibs;

	class iterator_type
		: public boost::iterator_facade<
			iterator_type, resource_type, boost::forward_traversal_tag,
			// Return copy instead of reference:
			resource_type> {
		typedef size_t index_value_type;
		typedef redman_manager_type manager_type;
		typedef typename manager_type::iterator_type underlying_iterator_type;

	public:
		typedef std::queue<wafer_map_type::const_iterator> queue_type;
		enum mode_type {
			PRESENT, AVAILABLE, ALLOCATED
		};

		iterator_type(
			allocation_type const& masked,
			allocation_type const& allocated,
			queue_type&& queue,
			mode_type mode);

	private:
		friend class boost::iterator_core_access;

		bool equal(iterator_type const& other) const;
		void increment();
		void check_maybe_increment();
		resource_type dereference() const;

		void pop();

		const mode_type mMode;
		allocation_type const& mMasked;
		allocation_type const& mAllocated;
		wafer_type mWafer;
		boost::shared_ptr<const manager_type> mCurrent;
		underlying_iterator_type mIter;
		queue_type mQueue;
	};

	iterator_type begin(typename iterator_type::mode_type mode) const;
	iterator_type end(typename iterator_type::mode_type mode) const;

public:
	/// Return an iterator to the beginning of all present Resources.
	iterator_type begin_present() const {
		return begin(iterator_type::PRESENT);
	}

	/// Return an iterator to the end of all present Resources.
	iterator_type end_present() const {
		return end(iterator_type::PRESENT);
	}

	/// Allow range-based-for iteration over all present Resources.
	iterable<iterator_type> present() const {
		return {begin_present(), end_present()};
	}

	/// Return an iterator to the beginning of all available Resources.
	iterator_type begin_available() const {
		return begin(iterator_type::AVAILABLE);
	}

	/// Return an iterator to the end of all available Resources.
	iterator_type end_available() const {
		return end(iterator_type::AVAILABLE);
	}

	/// Allow range-based-for iteration over all available Resources.
	iterable<iterator_type> available() const {
		return {begin_available(), end_available()};
	}

	/// Return an iterator to the beginning of all allocated Resources.
	iterator_type begin_allocated() const {
		return begin(iterator_type::ALLOCATED);
	}

	/// Return an iterator to the end of all allocated Resources.
	iterator_type end_allocated() const {
		return end(iterator_type::ALLOCATED);
	}

	/// Allow range-based-for iteration over all allocated Resources.
	iterable<iterator_type> allocated() const {
		return {begin_allocated(), end_allocated()};
	}
};

typedef Manager<halco::hicann::v2::HICANNGlobal> HICANNManager;
typedef Manager<halco::hicann::v2::FPGAGlobal> FPGAManager;

template class Manager<halco::hicann::v2::HICANNGlobal>;
template class Manager<halco::hicann::v2::FPGAGlobal>;

} // namespace resource
} // namespace marocco
