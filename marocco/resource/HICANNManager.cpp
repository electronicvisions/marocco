#include <tuple>
#include <stdexcept>
#include <tuple>
#include <utility>

#include "marocco/resource/HICANNManager.h"

namespace Co = HMF::Coordinate;

namespace marocco {
namespace resource {

HICANNManager::HICANNManager(
	boost::shared_ptr<redman::backend::Backend> backend,
	std::set<wafer_type> const& wafers)
	: mBackend(backend) {
	for (auto const& wafer : wafers) {
		load(wafer);
	}
}

void HICANNManager::reload() {
	std::set<wafer_type> wafers;

	for (auto const& pair : mWafers) {
		wafers.insert(pair.first);
	}

	mWafers.clear();

	for (auto const& wafer : wafers) {
		load(wafer);
	}
}

void HICANNManager::load(wafer_type const& wafer) {
	auto it = mWafers.find(wafer);

	if (it != mWafers.end())
		throw std::runtime_error("Wafer has already been loaded.");

	mWafers.insert(
	    std::make_pair(wafer, manager_type(mBackend, wafer)));
}

void HICANNManager::inject(manager_type const& wafer) {
	auto pair = mWafers.insert(std::make_pair(wafer.id(), wafer));
	if (!pair.second)
		throw std::runtime_error("Wafer has already been loaded.");
}

auto HICANNManager::wafers() const -> std::vector<wafer_type> {
	std::vector<wafer_type> ws;
	ws.reserve(mWafers.size());
	for (auto const& pair : mWafers) {
		ws.push_back(pair.first);
	}
	return ws;
}

bool HICANNManager::has(resource_type const& r) const {
	return (wafer_for(r) != mWafers.end()
	        && mMasked.find(r) == mMasked.end());
}

auto HICANNManager::wafer_for(resource_type const& r) const
    -> wafer_map_type::const_iterator {
	Co::Wafer wafer;
	Co::HICANNOnWafer hicann;
	std::tie(wafer, hicann) = r.split();

	auto it = mWafers.find(wafer);

	if (it != mWafers.end() && it->second.has(hicann))
		return it;

	return mWafers.end();
}

boost::shared_ptr<const redman::resources::Hicann>
HICANNManager::get(resource_type const& r) const {
	if (!has(r))
		throw std::runtime_error("HICANN not present.");

	auto it = wafer_for(r);
	return it->second.get(r.toHICANNOnWafer());
}

void HICANNManager::mask(resource_type const& r) {
	if (wafer_for(r) == mWafers.end())
		throw std::runtime_error("HICANN not present.");

	auto it = mMasked.insert(r);
	if (!it.second)
		throw std::runtime_error("HICANN has already been masked.");
}

void HICANNManager::unmask(resource_type const& r) {
	if (wafer_for(r) == mWafers.end())
		throw std::runtime_error("HICANN not present.");

	auto it = mMasked.find(r);
	if (it == mMasked.end())
		throw std::runtime_error("HICANN has not been masked yet.");

	mMasked.erase(it);
}

bool HICANNManager::masked(resource_type const& r) const {
	return mMasked.find(r) != mMasked.end();
}

void HICANNManager::allocate(resource_type const& r) {
	if (!has(r))
		throw std::runtime_error("HICANN not present.");

	auto it = mAllocated.insert(r);
	if (!it.second)
		throw std::runtime_error("HICANN has already been allocated.");
}

void HICANNManager::release(resource_type const& r) {
	if (!has(r))
		throw std::runtime_error("HICANN not present.");

	auto it = mAllocated.find(r);
	if (it == mAllocated.end())
		throw std::runtime_error("HICANN has not been allocated yet.");

	mAllocated.erase(it);
}

bool HICANNManager::available(resource_type const& r) const {
	return (has(r) && mAllocated.find(r) == mAllocated.end());
}

size_t HICANNManager::count_present() const {
	size_t available = 0;
	for (auto const& pair : mWafers) {
		available += pair.second.hicanns()->available();
	}
	return available - mMasked.size();
}

size_t HICANNManager::count_available() const {
	return count_present() - mAllocated.size();
}

size_t HICANNManager::count_allocated() const {
	return mAllocated.size();
}

auto HICANNManager::begin(iterator_type::mode_type mode) const -> iterator_type {
	iterator_type::queue_type queue;

	for (auto it = mWafers.cbegin(); it != mWafers.cend(); ++it) {
		queue.push(it);
	}

	return iterator_type{mMasked, mAllocated, std::move(queue), mode};
}

auto HICANNManager::end(iterator_type::mode_type mode) const -> iterator_type {
	return iterator_type{mMasked, mAllocated, iterator_type::queue_type{}, mode};
}

HICANNManager::iterator_type::iterator_type(
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

bool HICANNManager::iterator_type::equal(iterator_type const& other) const {
	return mMode == other.mMode
		&& mWafer == other.mWafer
		&& mCurrent == other.mCurrent
		&& mIter == other.mIter
		&& mQueue == other.mQueue;
}

void HICANNManager::iterator_type::increment() {
	++mIter;

	if (mIter == mCurrent->end())
		pop();

	check_maybe_increment();
}

void HICANNManager::iterator_type::check_maybe_increment() {
	while (mCurrent && mIter != mCurrent->end()) {
		auto hicann = dereference();
		auto masked = (mMasked.find(hicann) != mMasked.end());
		auto allocated = (mAllocated.find(hicann) != mAllocated.end());

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

auto HICANNManager::iterator_type::dereference() const -> resource_type {
	// if (!mCurrent)
	// 	throw std::runtime_error("Dereferencing end iterator.");
	return Co::HICANNGlobal(*mIter, mWafer);
}

void HICANNManager::iterator_type::pop() {
	if (mQueue.empty()) {
		mWafer = Co::Wafer{};
		mCurrent.reset();
		mIter = underlying_iterator_type{};
		return;
	}

	auto pair = *mQueue.front();
	mWafer = pair.first;
	mCurrent = pair.second.hicanns();
	mIter = mCurrent->begin();
	mQueue.pop();
}

} // namespace resource
} // namespace marocco
