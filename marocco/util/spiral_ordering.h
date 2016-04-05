#pragma once

#include <algorithm>
#include <cmath>

namespace marocco {

template <typename T>
class spiral_ordering {
	static constexpr float CENTER_X = T::x_type::min + T::x_type::size / 2. - 0.5;
	static constexpr float CENTER_Y = T::y_type::min + T::y_type::size / 2. - 0.5;

public:
	bool operator()(T const& lhs, T const& rhs) const {
		if (distance(lhs) != distance(rhs)) {
			return distance(lhs) < distance(rhs);
		}
		return angle(lhs) < angle(rhs);
	}

	float x(T const& val) const {
		return float(val.x()) - CENTER_X;
	}

	float y(T const& val) const {
		return float(val.y()) - CENTER_Y;
	}

	float angle(T const& val) const {
		constexpr float cos = std::cos(-M_PI/4.);
		constexpr float sin = std::sin(-M_PI/4.);
		float x_ = x(val) * cos - y(val) * sin;
		float y_ = x(val) * sin + y(val) * cos;

		return std::atan2(y_, x_);
	}

	float distance(T const& val) const {
		return std::max(std::abs(x(val)), std::abs(y(val)));
	}
}; // spiral_ordering

} // namespace marocco
