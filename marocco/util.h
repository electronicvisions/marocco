#pragma once
#include <stdexcept>

namespace marocco {

#define NEW_EXCEPTION_TYPE(NAME, BASE)                                                             \
	struct NAME : public BASE                                                                      \
	{                                                                                              \
		template <typename... F>                                                                   \
		NAME(F&&... args) : BASE(std::forward<F>(args)...)                                         \
		{                                                                                          \
		}                                                                                          \
	};

NEW_EXCEPTION_TYPE(Error, std::runtime_error)
NEW_EXCEPTION_TYPE(ResourceInUseError, Error)
NEW_EXCEPTION_TYPE(ResourceExhaustedError, Error)

#undef NEW_EXCEPTION_TYPE

} // namespace marocco
