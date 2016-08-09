#pragma once

#include <vector>
#ifndef PYPLUSPLUS
#include <unordered_map>
#endif // !PYPLUSPLUS

#include <boost/serialization/export.hpp>

#include "marocco/coordinates/BioNeuron.h"

namespace boost {
namespace serialization {
class access;
} // namespace serialization
} // namespace boost

namespace marocco {
namespace parameter {
namespace results {

/**
 * @brief Stores untransformed spike times of bio neurons.
 * These are extracted / computed from the pynn / euter description and stored in this
 * intermediate data structure to enable modification of the spike times of individual
 * neurons after mapping.  E.g. access via sthal only allows spikes to be cleared on a
 * per-FPGA basis.
 * @note Spikes are stored in an unsorted fashion, as sorting will have to be performed by
 *       sthal anyways, when merging spike trains of multiple neurons.
 */
class SpikeTimes
{
public:
	typedef std::vector<double> spikes_type;

	spikes_type const& get(BioNeuron const& neuron) const;

	void clear(BioNeuron const& neuron);

	void set(BioNeuron const& neuron, spikes_type const& spikes);

#ifndef PYPLUSPLUS
	void set(BioNeuron const& neuron, spikes_type&& spikes);
#endif // !PYPLUSPLUS

	void add(BioNeuron const& neuron, spikes_type const& spikes);

	void add(BioNeuron const& neuron, double const& time);

	bool empty() const;

	size_t size() const;

private:
#ifndef PYPLUSPLUS
	std::unordered_map<BioNeuron, spikes_type> m_spikes;
#endif // !PYPLUSPLUS

	friend class boost::serialization::access;
	template <typename Archiver>
	void serialize(Archiver& ar, const unsigned int /* version */);
}; // SpikeTimes

} // namespace results
} // namespace parameter
} // namespace marocco

BOOST_CLASS_EXPORT_KEY(::marocco::parameter::results::SpikeTimes)
