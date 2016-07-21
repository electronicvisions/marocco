#pragma once

#include <stdexcept>

namespace marocco {

#ifndef PYPLUSPLUS
#define NEW_EXCEPTION_TYPE(NAME, BASE)                                                             \
	struct NAME : public BASE                                                                      \
	{                                                                                              \
		NAME(std::string const& what) : BASE(what)                                                 \
		{                                                                                          \
		}                                                                                          \
		NAME(char const* what) : BASE(what)                                                        \
		{                                                                                          \
		}                                                                                          \
	};
#else
#define NEW_EXCEPTION_TYPE(NAME, BASE)                                                             \
	struct NAME : public BASE                                                                      \
	{                                                                                              \
		NAME(std::string const& what) : BASE(what)                                                 \
		{                                                                                          \
		}                                                                                          \
	};
#endif // !PYPLUSPLUS

NEW_EXCEPTION_TYPE(Error, std::runtime_error)
NEW_EXCEPTION_TYPE(ResourceInUseError, Error)
NEW_EXCEPTION_TYPE(ResourceExhaustedError, Error)
NEW_EXCEPTION_TYPE(ResourceNotPresentError, Error)

#undef NEW_EXCEPTION_TYPE

} // namespace marocco
