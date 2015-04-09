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
#include <vector>
#include <iosfwd>
#include <set>

#include "marocco/config.h"
#include "marocco/graph.h"
#include "marocco/resource/HICANNManager.h"

#include "pywrap/compat/hash.hpp"

namespace marocco {
namespace placement {

class Result; // fwd dcl


/**
 * @brief representation of global L1 Address
 **/
struct hw_id
{
	HMF::Coordinate::HICANNGlobal hicann;
	HMF::Coordinate::OutputBufferOnHICANN outb;
	HMF::HICANN::L1Address addr;

	bool operator== (hw_id const& k) const;
	bool operator!= (hw_id const& k) const;
	std::ostream& operator<< (std::ostream& os) const;
};

/**
 * @brief representation of global pynn neuron
 **/
struct bio_id
{
	size_t pop;    //<! population id
	size_t neuron; //<! relative neuron address

	bool operator== (bio_id const& v) const;
	bool operator!= (bio_id const& v) const;
	std::ostream& operator<< (std::ostream& os) const;
};

} // namespace placement
} // namespace marocco

namespace std {
template<>
struct hash<marocco::placement::hw_id>
{
	typedef marocco::placement::hw_id type;
	size_t operator()(type const & t) const;
};

template<>
struct hash<marocco::placement::bio_id>
{
	typedef marocco::placement::bio_id type;
	size_t operator()(type const & t) const;
};
} // std


namespace marocco {
namespace placement {

/**
 * @class LookupTable
 *
 * @brief contains the actual reverse mapping.
 **/
class LookupTable
{
public:
	LookupTable() = default;
	LookupTable(LookupTable const&) = default;
	LookupTable(LookupTable&&) = default;
	LookupTable(Result const &result, resource_manager_t const &mgr, graph_t const &graph);

	// hw to bio transformation
	bio_id&       operator[] (hw_id const& key);
	bio_id&       at(hw_id const& key);
	bio_id const& at(hw_id const& key) const;

	// bio to hw transformation
	std::vector< hw_id > &operator[](bio_id const &key);
	std::vector< hw_id > &at(bio_id const &key);
	std::vector< hw_id > const &at(bio_id const &key) const;

	size_t size() const;
	bool empty() const;

private:
	typedef std::unordered_map< hw_id, bio_id > hw_to_bio_map_type;
	typedef std::unordered_map< bio_id, std::vector< hw_id > > bio_to_hw_map_type;
	typedef std::unordered_map< bio_id, std::vector< HMF::Coordinate::NeuronGlobal > >
	    bio_to_denmem_map_type;

	hw_to_bio_map_type mHw2BioMap;
	bio_to_hw_map_type mBio2HwMap;
	bio_to_denmem_map_type mBio2DenmemMap;
};

} // namespace placement
} // namespace marocco
