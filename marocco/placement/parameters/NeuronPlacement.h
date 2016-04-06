#pragma once

#include <boost/serialization/export.hpp>

namespace boost {
namespace serialization {
class access;
} // namespace serialization
} // namespace boost

namespace marocco {
namespace placement {
namespace parameters {

/**
 * @brief Ensure that the given neuron size is valid.
 * @throw std::invalid_argument If it does not fulfill the requirements.
 */
void check_neuron_size(size_t size);

class NeuronPlacement {
public:
	typedef size_t size_type;

	NeuronPlacement();

	/**
	 * @brief Default size of a logical neuron.
	 * @throw std::invalid_argument If given neuron size is invalid.
	 */
	void default_neuron_size(size_type size);
	size_type default_neuron_size() const;

	/**
	 * @brief Reserve rightmost OutputBuffer to be used for DNC input and bg events.
	 * The rationale behind this is that we need events with L1 address zero for locking
	 * repeaters and synapse drivers.  In principle those events could be provided through
	 * the DNC input;  But as this sets in too late and/or is too short, the current
	 * approach is to use the background generator connected to this output buffer (which
	 * is forwarded 1-to-1 to the corresponding DNC merger).
	 */
	void restrict_rightmost_neuron_blocks(bool enable);
	bool restrict_rightmost_neuron_blocks() const;

	/**
	 * @brief Try to minimize number of required sending repeaters by restricting number
	 *        of logical neurons placed on each neuron block.
	 * @note This only works when automatic placement is used and applies to neuron sizes
	 *       2, 4 and 8.
	*/
	void minimize_number_of_sending_repeaters(bool enable);
	bool minimize_number_of_sending_repeaters() const;

private:
	size_type m_default_neuron_size;
	bool m_restrict_rightmost_neuron_blocks;
	bool m_minimize_number_of_sending_repeaters;

	friend class boost::serialization::access;
	template <typename Archive>
	void serialize(Archive& ar, unsigned int const /* version */);
}; // NeuronPlacement

} // namespace parameters
} // namespace placement
} // namespace marocco

BOOST_CLASS_EXPORT_KEY(::marocco::placement::parameters::NeuronPlacement)
