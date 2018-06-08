#pragma once

/**
 * Reverse Mapping
 *
 * For the interpretation of hardware results we need the mapping in reverse
 * order, meaning from hardware value to pynn value.
 * Therefore, we need to generate this mapping at the time of forward mapping.
 * And propagate it to the reverse transformation instance.
 *
 **/

#include <unordered_map>
#include <iosfwd>
#include "marocco/config.h"
#include "marocco/graph.h"
#include "pywrap/compat/hash.hpp"

namespace marocco {
namespace placement {

class Result; // fwd dcl

/**
 * @brief representation of global L1 Address
 **/
struct RevKey
{
	HMF::Coordinate::HICANNGlobal hicann;
	HMF::Coordinate::OutputBufferOnHICANN outb;
	HMF::HICANN::L1Address addr;

	bool operator== (RevKey const& k) const;
	bool operator!= (RevKey const& k) const;
	std::ostream& operator<< (std::ostream& os) const;
};

/**
 * @brief representation of global pynn neuron
 **/
struct RevVal
{
	uint16_t pop;    //<! population id
	uint16_t neuron; //<! relative neuron addresse

	bool operator== (RevVal const& v) const;
	bool operator!= (RevVal const& v) const;
	std::ostream& operator<< (std::ostream& os) const;
};

} // namespace placement
} // namespace marocco

namespace std {
template<>
struct hash<marocco::placement::RevKey>
{
	typedef marocco::placement::RevKey type;
	size_t operator()(type const & t) const;
};
} // std


namespace marocco {
namespace placement {

/**
 * @class ReverseMapping
 *
 * @brief contains the actual reverse mapping.
 **/
class ReverseMapping
{
public:
	typedef size_t size_type;
	typedef RevKey key_type;
	typedef RevVal mapped_type;
	typedef std::unordered_map<key_type, mapped_type> type;

	ReverseMapping() = default;
	ReverseMapping(ReverseMapping const&) = default;
	ReverseMapping(ReverseMapping&&) = default;
	ReverseMapping(Result const& result,
				   resource_manager_t const& mgr,
				   graph_t const& graph);

	mapped_type&       operator[] (key_type const& key);

	mapped_type&       at(key_type const& key);
	mapped_type const& at(key_type const& key) const;

	size_type size() const;
	bool empty() const;

private:
	type mMapping;
};

} // namespace placement
} // namespace marocco
