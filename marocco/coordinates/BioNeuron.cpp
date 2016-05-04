#include "marocco/coordinates/BioNeuron.h"

#include <boost/functional/hash.hpp>
#include <boost/serialization/nvp.hpp>

namespace marocco {

BioNeuron::BioNeuron()
	: m_population(), m_neuron_index()
{
}

BioNeuron::BioNeuron(vertex_descriptor population, size_t neuron_index)
	: m_population(population), m_neuron_index(neuron_index)
{
}

auto BioNeuron::population() const -> vertex_descriptor
{
	return m_population;
}

size_t BioNeuron::neuron_index() const
{
	return m_neuron_index;
}

std::ostream& operator<<(std::ostream& os, BioNeuron const& nrn)
{
	os << "BioNeuron(" << nrn.population() << ", " << nrn.neuron_index() << ")";
	return os;
}

bool operator==(BioNeuron const & lhs, BioNeuron const& rhs)
{
	return lhs.population() == rhs.population() && lhs.neuron_index() == rhs.neuron_index();
}

size_t hash_value(BioNeuron const& nrn)
{
	size_t hash = 0;
	boost::hash_combine(hash, nrn.population());
	boost::hash_combine(hash, nrn.neuron_index());
	return hash;
}

template <typename Archiver>
void BioNeuron::serialize(Archiver& ar, const unsigned int /* version */)
{
	using namespace boost::serialization;
	// clang-format off
	ar & make_nvp("population", m_population)
	   & make_nvp("neuron_index", m_neuron_index);
	// clang-format on
}

} // namespace marocco

BOOST_CLASS_EXPORT_IMPLEMENT(::marocco::BioNeuron)

#include "boost/serialization/serialization_helper.tcc"
EXPLICIT_INSTANTIATE_BOOST_SERIALIZE(::marocco::BioNeuron)
