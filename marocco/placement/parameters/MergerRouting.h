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

class MergerRouting {
public:
	MergerRouting();

	PYPP_CLASS_ENUM(Strategy)
	{
		/**
		 * @brief Try to merge adjacent neuron blocks.
		 * This option reduces the number of DNC mergers used for hardware neurons,
		 * thereby providing more resources for external spike input.
		 */
		minimize_number_of_sending_repeaters,
		one_to_one
	};

	void strategy(Strategy value);
	Strategy strategy() const;

private:
	Strategy m_strategy;

	friend class boost::serialization::access;
	template <typename Archive>
	void serialize(Archive& ar, unsigned int const /* version */);
}; // MergerRouting

} // namespace parameters
} // namespace placement
} // namespace marocco

BOOST_CLASS_EXPORT_KEY(::marocco::placement::parameters::MergerRouting)
