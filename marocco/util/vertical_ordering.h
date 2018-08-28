#pragma once

#include <algorithm>
#include <cmath>
#include <limits>
#ifdef __cpp_impl_three_way_comparison
#include <compare>
#endif

namespace marocco {

template <typename T>
class vertical_ordering
{
	static constexpr float DEFAULT_CENTER_X = T::x_type::min + T::x_type::size / 2. - 0.5;
	static constexpr float DEFAULT_CENTER_Y = T::y_type::min + T::y_type::size / 2. - 0.5;

public:
	vertical_ordering(
	    float center_x = DEFAULT_CENTER_X, float center_y = DEFAULT_CENTER_Y, float epsilon = 0) :
	    m_center_x(center_x + epsilon),
	    m_center_y(center_y + epsilon)
	// adding epsilon to prefer top over bottom and left over right, or the other way around.
	// else there are up to 4 hicanns having the same priority.
	// NOTE: !the user has to decide how to handle this cases!
	{}

	bool operator()(T const& lhs, T const& rhs) const
	{
#ifdef __cpp_impl_three_way_comparison
		return lhs < rhs;
#else
		if (distance_x(lhs) != distance_x(rhs)) {
			return distance_x(lhs) < distance_x(rhs);
		}
		if (distance_y(lhs) != distance_y(rhs)) {
			return distance_y(lhs) < distance_y(rhs);
		}
		return lhs.toEnum() < rhs.toEnum();
#endif
	}

#ifdef __cpp_impl_three_way_comparison
	std::strong_ordering operator<=>(T const& lhs, T const& rhs) const
	{
		if (distance_x(lhs) != distance_x(rhs)) {
			return distance_x(lhs) <=> distance_x(rhs);
		}
		if (distance_y(lhs) != distance_y(rhs)) {
			return distance_y(lhs) <=> distance_y(rhs);
		}
		return lhs.toEnum() <=> rhs.toEnum();
	}
#endif

	float x(T const& val) const { return float(val.x()) - m_center_x; }

	float y(T const& val) const { return float(val.y()) - m_center_y; }

	float distance_y(T const& val) const { return std::abs(y(val)); }

	float distance_x(T const& val) const { return std::abs(x(val)); }

private:
	float const m_center_x;
	float const m_center_y;

}; // spiral_ordering

} // namespace marocco
