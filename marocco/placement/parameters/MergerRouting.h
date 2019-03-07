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

	PYPP_CLASS_ENUM(Strategy){
	    // clang-format off
	    /**
	     * @brief Try to merge adjacent neuron blocks.
	     * This option reduces the number of DNC mergers used for hardware neurons,
	     * thereby reducing the L1 usage.
	     * external spikes still require BG generators from the top of the merger tree, thus
	     * external spikes do not profit from this mapping
	     */
	    minimize_number_of_sending_repeaters,
	    /**
	     * @brief Maps Neurons blocks straight to DNCMergers, thus more DNCMergers get used,
	     * it is good for neurons with high rate.
	     */
	    one_to_one,
	    /**
	     * @brief Try to merge adjacent neuron blocks if SynapseDriverChainLengths at the target
	     * is not violated.
	     * This reduces L1 resources, while it does not provoke loss on the targets.
	     * It should be used for neurons with low rate.
	     */
	    minimize_as_possible
	    // clang-format on
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
