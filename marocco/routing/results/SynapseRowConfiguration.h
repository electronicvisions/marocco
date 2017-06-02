#pragma once

#include <boost/serialization/export.hpp>

#include "hal/Coordinate/Neuron.h"
#include "hal/Coordinate/Relations.h"
#ifndef PYPLUSPLUS
#include "hal/Coordinate/typed_array.h"
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

	SynapseRowConfiguration(HMF::Coordinate::SynapticInputOnNeuron const& synaptic_input);

	address_type const& address(HMF::Coordinate::Parity const& parity) const;

	void set_address(HMF::Coordinate::Parity const& parity, address_type const& address);

	HMF::Coordinate::SynapticInputOnNeuron const& synaptic_input() const;

	void set_synaptic_input(HMF::Coordinate::SynapticInputOnNeuron const& synaptic_input);

private:
	HMF::Coordinate::SynapticInputOnNeuron m_synaptic_input;
#ifndef PYPLUSPLUS
	HMF::Coordinate::typed_array<address_type, HMF::Coordinate::Parity> m_address;
#endif // !PYPLUSPLUS

	friend class boost::serialization::access;
	template <typename Archiver>
	void serialize(Archiver& ar, const unsigned int /* version */);
}; // SynapseRowConfiguration

} // namespace results
} // namespace routing
} // namespace marocco

BOOST_CLASS_EXPORT_KEY(::marocco::routing::results::SynapseRowConfiguration)
