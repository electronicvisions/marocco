#pragma once

#include <boost/serialization/export.hpp>

#include "halco/hicann/v2/neuron.h"
#include "halco/common/relations.h"
#ifndef PYPLUSPLUS
#include "halco/common/typed_array.h"
#endif // !PYPLUSPLUS
#include "hal/HICANN/DriverDecoder.h"

namespace boost {
namespace serialization {
class access;
} // namespace serialization
} // namespace boost

namespace marocco {
namespace routing {
namespace results {

class SynapseRowConfiguration {
public:
	typedef HMF::HICANN::DriverDecoder address_type;

	SynapseRowConfiguration();

	SynapseRowConfiguration(halco::hicann::v2::SynapticInputOnNeuron const& synaptic_input);

	address_type const& address(halco::common::Parity const& parity) const;

	void set_address(halco::common::Parity const& parity, address_type const& address);

	halco::hicann::v2::SynapticInputOnNeuron const& synaptic_input() const;

	void set_synaptic_input(halco::hicann::v2::SynapticInputOnNeuron const& synaptic_input);

private:
	halco::hicann::v2::SynapticInputOnNeuron m_synaptic_input;
#ifndef PYPLUSPLUS
	halco::common::typed_array<address_type, halco::common::Parity> m_address;
#endif // !PYPLUSPLUS

	friend class boost::serialization::access;
	template <typename Archiver>
	void serialize(Archiver& ar, const unsigned int /* version */);
}; // SynapseRowConfiguration

} // namespace results
} // namespace routing
} // namespace marocco

BOOST_CLASS_EXPORT_KEY(::marocco::routing::results::SynapseRowConfiguration)
