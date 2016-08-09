#include "marocco/parameter/results/SpikeTimes.h"

#include <iterator>
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/vector.hpp>

#include "boost/serialization/unordered_map.h"

namespace marocco {
namespace parameter {
namespace results {

auto SpikeTimes::get(BioNeuron const& neuron) const -> spikes_type const&
{
	auto const it = m_spikes.find(neuron);
	if (it != m_spikes.end()) {
		return it->second;
	}

	static const spikes_type empty{};
	return empty;
}

void SpikeTimes::clear(BioNeuron const& neuron)
{
	m_spikes.erase(neuron);
}

void SpikeTimes::set(BioNeuron const& neuron, spikes_type const& spikes)
{
	m_spikes[neuron] = spikes;
}

void SpikeTimes::set(BioNeuron const& neuron, spikes_type&& spikes)
{
	m_spikes[neuron] = std::move(spikes);
}

void SpikeTimes::add(BioNeuron const& neuron, spikes_type const& spikes)
{
	auto& existing = m_spikes[neuron];
	existing.reserve(existing.size() + spikes.size());
	std::copy(spikes.begin(), spikes.end(), std::back_inserter(existing));
}

void SpikeTimes::add(BioNeuron const& neuron, double const& time)
{
	m_spikes[neuron].push_back(time);
}

bool SpikeTimes::empty() const
{
	return m_spikes.empty();
}

size_t SpikeTimes::size() const
{
	return m_spikes.size();
}

template <typename Archiver>
void SpikeTimes::serialize(Archiver& ar, const unsigned int /* version */)
{
	using namespace boost::serialization;
	// clang-format off
	ar & make_nvp("spikes", m_spikes);
	// clang-format on
}

} // namespace results
} // namespace parameter
} // namespace marocco

BOOST_CLASS_EXPORT_IMPLEMENT(::marocco::parameter::results::SpikeTimes)

#include "boost/serialization/serialization_helper.tcc"
EXPLICIT_INSTANTIATE_BOOST_SERIALIZE(::marocco::parameter::results::SpikeTimes)
