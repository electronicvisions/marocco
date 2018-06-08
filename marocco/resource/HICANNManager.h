#pragma once

#include <queue>
#include <map>
#include <set>
#include <unordered_set>
#include <unordered_map>

#include <boost/serialization/shared_ptr.hpp>
#include <boost/iterator/iterator_facade.hpp>

#include "hal/Coordinate/HMFGeometry.h"
#include "marocco/util.h"
#include "redman/resources/Wafer.h"

namespace redman {
class Hicann;
namespace backend {
class Backend;
}
}

namespace marocco {
namespace resource {
/** Overlay that can be used to track allocated HICANNs at runtime. */
class HICANNManager {
public:
	typedef HMF::Coordinate::HICANNGlobal resource_type;

private:
	typedef HMF::Coordinate::Wafer wafer_type;
	typedef redman::resources::WaferWithBackend manager_type;
	typedef std::map<wafer_type, const manager_type> wafer_map_type;
	typedef std::unordered_set<resource_type> allocation_type;

public:
	HICANNManager(
		boost::shared_ptr<redman::backend::Backend> backend,
		std::set<wafer_type> const& wafers = {});

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

	/** Check whether this HICANN is present.
	 * This is the case if the given HICANN is managed by this instance
	 * (i.e. its wafer has been loaded) and has not been disabled in
	 * the resource management.
	 * In addition to the data provided by the resource management a
	 * HICANN can be marked as absent using `mask()`.
	 */
	bool has(resource_type const& hicann) const;

	/** Mark the given HICANN as being absent, e.g. due to use in a
	 * different process.
	 * \throws std::runtime_error If HICANN is disabled in the resource
	 *         management or when trying to mark an already masked HICANN.
	 */
	void mask(resource_type const& hicann);

	/** Revert the operation done by `mask()`.
	 * Its presence (see `has()`) is then only determined by the
	 * underlying defect data.
	 * \throws std::runtime_error If HICANN is disabled in the resource
	 *         management or if this HICANN is not masked.
	 */
	void unmask(resource_type const& hicann);

	/** Check whether the given HICANN has been masked (see `mask()`).
	 */
	bool masked(resource_type const& hicann) const;

	/** Load the resource manager for the given HICANN.
	 * \throws std::runtime_error If `has()` fails for this HICANN.
	 */
	boost::shared_ptr<const redman::resources::Hicann>
	get(resource_type const& hicann) const;

	/** Mark the given HICANN as being 'in use' (not available anymore).
	 * \throws std::runtime_error If `has()` fails for this HICANN
	 *         or if trying to allocate twice.
	 */
	void allocate(resource_type const& hicann);

	/** Release HICANN into pool of available HICANNs.
	 * \throws std::runtime_error If `has()` fails for this HICANN or if it
	 *         has not been allocated yet.
	 */
	void release(resource_type const& hicann);

	/** Check whether the given HICANN is available.
	 * In addition to `has()` this checks whether the given HICANN has already
	 * been allocated.
	 */
	bool available(resource_type const& hicann) const;

	/// Return the number of available HICANNs.
	size_t count_available() const;

	/// Return the number of present HICANNs.
	size_t count_present() const;

	/// Return the number of allocated HICANNs.
	size_t count_allocated() const;

	boost::shared_ptr<redman::backend::Backend> backend() const { return mBackend; }

protected:
	friend class AHICANNManager;

	/** Reload all data from the backend (for use in tests).
	 * Does not touch the set of allocated HICANNs.  Note that wafers
	 * added via `inject()` will also be reloaded from the backend.
	 */
	void reload();

private:
	/** Returns an iterator to the wafer map entry belonging to the given
	 * HICANN if the checks outlined in the description of `has()` succeed.
	 * Else a past-the-end iterator is returned.
	 */
	wafer_map_type::const_iterator wafer_for(resource_type const& hicann) const;

	boost::shared_ptr<redman::backend::Backend> mBackend;
	wafer_map_type mWafers;
	allocation_type mMasked;
	allocation_type mAllocated;

	class iterator_type
		: public boost::iterator_facade<
			iterator_type, resource_type, boost::forward_traversal_tag,
			// Return copy instead of reference:
			resource_type> {
		typedef size_t index_value_type;
		typedef redman::resources::components::Hicanns manager_type;
		typedef manager_type::iterator_type underlying_iterator_type;

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

	iterator_type begin(iterator_type::mode_type mode) const;
	iterator_type end(iterator_type::mode_type mode) const;

public:
	/// Return an iterator to the beginning of all present HICANNs.
	iterator_type begin_present() const {
		return begin(iterator_type::PRESENT);
	}

	/// Return an iterator to the end of all present HICANNs.
	iterator_type end_present() const {
		return end(iterator_type::PRESENT);
	}

	/// Allow range-based-for iteration over all present HICANNs.
	iterable<iterator_type> present() const {
		return {begin_present(), end_present()};
	}

	/// Return an iterator to the beginning of all available HICANNs.
	iterator_type begin_available() const {
		return begin(iterator_type::AVAILABLE);
	}


	/// Return an iterator to the end of all available HICANNs.
	iterator_type end_available() const {
		return end(iterator_type::AVAILABLE);
	}

	/// Allow range-based-for iteration over all available HICANNs.
	iterable<iterator_type> available() const
	{
		return {begin_available(), end_available()};
	}

	/// Return an iterator to the beginning of all allocated HICANNs.
	iterator_type begin_allocated() const {
		return begin(iterator_type::ALLOCATED);
	}

	/// Return an iterator to the end of all allocated HICANNs.
	iterator_type end_allocated() const {
		return end(iterator_type::ALLOCATED);
	}

	/// Allow range-based-for iteration over all allocated HICANNs.
	iterable<iterator_type> allocated() const {
		return {begin_allocated(), end_allocated()};
	}
};

} // namespace resource
} // namespace marocco
