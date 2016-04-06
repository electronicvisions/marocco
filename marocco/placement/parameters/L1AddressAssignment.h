#pragma once

#include <boost/serialization/export.hpp>

#include "pywrap/compat/macros.hpp"

namespace boost {
namespace serialization {
class access;
} // namespace serialization
} // namespace boost

namespace marocco {
namespace placement {
namespace parameters {

class L1AddressAssignment {
public:
	L1AddressAssignment();

	PYPP_CLASS_ENUM(Strategy)
	{
		high_first,
		low_first,
		alternating
	};

	void strategy(Strategy value);
	Strategy strategy() const;

private:
	Strategy m_strategy;

	friend class boost::serialization::access;
	template <typename Archive>
	void serialize(Archive& ar, unsigned int const /* version */);
}; // L1AddressAssignment

} // namespace parameters
} // namespace placement
} // namespace marocco

BOOST_CLASS_EXPORT_KEY(::marocco::placement::parameters::L1AddressAssignment)
