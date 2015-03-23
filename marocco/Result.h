#pragma once

#include <typeinfo>

namespace marocco {

struct BaseResult {
	virtual ~BaseResult() {}
};

template<typename T>
T const& result_cast(BaseResult const& res)
{
	try {
		return dynamic_cast<T const&>(res);
	} catch(std::bad_cast const& e) {
		throw; // rethrow;
	}
}

template<typename T>
T& result_cast(BaseResult& res)
{
	BaseResult const& t = res;
	return const_cast<T&>(result_cast<T>(t));
}

} // namespace marocco
