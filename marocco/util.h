#pragma once

#include <utility>
#include <stdexcept>

namespace marocco {

class Error : public std::runtime_error {};
class ResourceInUseError : public Error {};

template<typename Iterator>
class iterable {
public:
	template <typename It>
	iterable(It begin, It end)
	    : mBegin(begin), mEnd(end) {}

	Iterator begin() const { return mBegin; }
	Iterator end()   const { return mEnd; }

private:
	Iterator const mBegin;
	Iterator const mEnd;
};

template <typename It>
inline iterable<It> make_iterable(std::pair<It, It> const& v) {
	return iterable<It>(v.first, v.second);
}

template <typename It>
inline iterable<It> make_iterable(It beg, It end) {
	return iterable<It>(beg, end);
}

} // namespace marocco
