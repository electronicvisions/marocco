#include "marocco/placement/OutputBufferMapping.h"

namespace marocco {
namespace placement {

//size_t const OutputBufferMapping::CAPACITY =
	//OutputBufferMapping::index::end;

OutputBufferMapping::OutputBufferMapping() :
	mMapping(),
	mMode()
{
	// TODO: use defect management
	for (auto& v : mMapping)
		v.second = CAPACITY;
	for (auto& v : mMode)
		v = Mode::INPUT;
}

OutputBufferMapping::list const&
OutputBufferMapping::at(index const& idx) const
{
	return mMapping.at(idx).first;
}

OutputBufferMapping::list const&
OutputBufferMapping::operator[] (index const& idx) const
{
	return mMapping[idx].first;
}

void OutputBufferMapping::insert(index const& idx, assign const & a)
{
	auto& v = mMapping[idx];
	if (a.bio().size() > v.second)
		throw std::runtime_error("out of free neurons");

	v.second -= a.bio().size();
	v.first.push_back(a);
}

bool OutputBufferMapping::any() const
{
	typedef std::pair<list, size_t> value;
	return std::any_of(mMapping.begin(), mMapping.end(),
		[](value const& l) { return !l.first.empty(); });
}

bool OutputBufferMapping::any(index const& outb)
{
	bool any_ = !mMapping[outb].first.empty();
	if (any_ && mMode[outb] == INPUT) {
		throw std::runtime_error("incompatible mode");
	}
	return any_;
}

size_t OutputBufferMapping::available(index const& idx)
{
	return mMapping[idx].second;
}

size_t OutputBufferMapping::available() const
{
	size_t cnt = 0;
	for (auto const& v : mMapping)
		cnt += v.second;
	return  cnt;
}

void OutputBufferMapping::setMode(index const& outb, Mode m)
{
	mMode[outb] = m;
}

OutputBufferMapping::Mode OutputBufferMapping::getMode(index const& outb) const
{
	return mMode[outb];
}

bool OutputBufferMapping::empty(index const& outb) const
{
	bool empty_ = mMapping[outb].second == CAPACITY;
	if (empty_ && mMode[outb] == OUTPUT) {
		throw std::runtime_error("incompatible mode");
	}
	return empty_;
}

AddressPool::pool_type
OutputBufferMapping::popAddresses(index const& ob,
                                  size_t N,
                                  pymarocco::PyMarocco::L1AddressAssignment
                                  l1_address_assignment)
{
	AddressPool& pool = mAddresses[ob];
	if (pool.size() < N) {
		std::cout << ob << " " << N;
		throw std::runtime_error("not enough L1 Addresses");
	}

	using pymarocco::PyMarocco;

	AddressPool::pool_type addr(N);
	size_t n=0;
	for (auto& val : addr) {

		switch(l1_address_assignment) {

		case PyMarocco::L1AddressAssignment::HighFirst:
			val = pool.pop_back();
			break;

		case PyMarocco::L1AddressAssignment::LowFirst:
			val = pool.pop_front();
			break;

		// works only when N > 1
		// otherwise, Alternate behaves like HighFirst
		case PyMarocco::L1AddressAssignment::Alternate:

			if(n%2) {
				val = pool.pop_front();
			} else {
				val = pool.pop_back();
			}

			break;

		default:
			throw std::runtime_error("Unknown L1 address assignment strategy");

		}

		++n;

	}
	return addr;
}

bool OutputBufferMapping::onlyOutput() const {

	return !std::any_of(mMode.begin(),
						mMode.end(),
						[](Mode m) { return m == Mode::INPUT; });

}

bool OutputBufferMapping::onlyInput() const {

	return !std::any_of(mMode.begin(),
						mMode.end(),
						[](Mode m) { return m == Mode::OUTPUT; });

}

} // namespace placement
} // namespace marocco
