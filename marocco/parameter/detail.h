#pragma once

namespace marocco {
namespace parameter {
namespace detail {

#define HAS_PARAMETER(PARAMETER) \
	template <typename T> \
	class has_##PARAMETER { \
	private: \
		template <typename U> \
		static constexpr bool \
		fun(decltype(&U::CellParameters::PARAMETER)* = 0) { return true; } \
		template <typename U> \
		static constexpr bool fun(...) { return false; } \
	public: \
		enum : bool { value = fun<T>(nullptr) }; \
	};

HAS_PARAMETER(record_v)
HAS_PARAMETER(record_spikes)
HAS_PARAMETER(v_reset)

#undef HAS_PARAMETER

} // namespace detail
} // namespace parameter
} // namespace marocco
