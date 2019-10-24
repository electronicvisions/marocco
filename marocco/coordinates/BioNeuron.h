#pragma once

#include <iostream>

#include <boost/operators.hpp>
#include <boost/serialization/export.hpp>

#ifndef PYPLUSPLUS
#include <functional>
#else
#include "pywrap/compat/hash.hpp"
#endif // !PYPLUSPLUS

namespace boost {
namespace serialization {
class access;
} // namespace serialization
} // namespace boost

namespace marocco {

/**
 * @brief Represents a single neuron of a population.
 * This class is used in the neuron placement output to associate logical neurons on the
 * hardware to single biological neurons via a neuron index and a vertex descriptor of the
 * bio graph (which holds populations as vertices).
 * While this necessitates an additional translation from euter id to vertex descriptor
 * for handling user queries, it allows for faster and more convenient lookup in other
 * parts of the mapping (i.e. without looking up properties in the graph).
 */
class BioNeuron : boost::equality_comparable<BioNeuron> {
	typedef size_t vertex_descriptor;

public:
	BioNeuron();
	BioNeuron(vertex_descriptor population, size_t neuron_index);

	// vertex descriptor of the BioGraph, coincides with euter_id; cf. BioGraph.cpp
	vertex_descriptor population() const;
	size_t neuron_index() const;

private:
	vertex_descriptor m_population;
	size_t m_neuron_index;

	friend class boost::serialization::access;
	template <typename Archiver>
	void serialize(Archiver& ar, const unsigned int /* version */);
}; // BioNeuron

std::ostream& operator<<(std::ostream& os, BioNeuron const& nrn);
bool operator==(BioNeuron const & lhs, BioNeuron const& rhs);
size_t hash_value(BioNeuron const& nrn);

} // namespace marocco

BOOST_CLASS_EXPORT_KEY(::marocco::BioNeuron)

namespace std {

template <>
struct hash<marocco::BioNeuron>
{
	size_t operator()(marocco::BioNeuron const& nrn) const
	{
		return hash_value(nrn);
	}
};

} // namespace std
