#pragma once

#include <array>
#include <iosfwd>
#include <boost/serialization/export.hpp>

#include "halco/hicann/v2/neuron.h"
#ifndef PYPLUSPLUS
#include "halco/common/typed_array.h"
#endif // !PYPLUSPLUS
#include "pywrap/compat/array.hpp"

#include "marocco/coordinates/SynapseType.h"

namespace boost {
namespace serialization {
class access;
} // namespace serialization
} // namespace boost

namespace marocco {
namespace routing {
namespace results {

/**
 * @brief Mapping of synaptic inputs to the synapse types that will be routed there.
 * @see marocco::routing::SynapseTargetMapping
 */
class SynapticInputs {
public:
#ifndef PYPLUSPLUS
	typedef halco::common::typed_array<SynapseType, halco::hicann::v2::SynapticInputOnNeuron> value_type;
	typedef halco::common::typed_array<value_type, halco::hicann::v2::NeuronOnHICANN> container_type;
#endif // !PYPLUSPLUS

	/**
	 * @brief Construct a mapping with all synapse types defaulting to \c SynapseType::none.
	 */
	SynapticInputs();

#ifndef PYPLUSPLUS
	value_type& operator[](halco::hicann::v2::NeuronOnHICANN const& neuron);
	value_type const& operator[](halco::hicann::v2::NeuronOnHICANN const& neuron) const;
#endif // !PYPLUSPLUS

	/// Checks whether synapse types are the same for top and bottom neurons.
	bool is_horizontally_symmetrical() const;

private:
#ifndef PYPLUSPLUS
	container_type m_neurons;
#endif // !PYPLUSPLUS

	friend class boost::serialization::access;
	template <typename Archiver>
	void serialize(Archiver& ar, unsigned int const /*version*/);
}; // SynapticInputs

std::ostream& operator<<(std::ostream& os, SynapticInputs const& synaptic_inputs);

} // namespace results
} // namespace routing
} // namespace marocco

BOOST_CLASS_EXPORT_KEY(::marocco::routing::results::SynapticInputs)
