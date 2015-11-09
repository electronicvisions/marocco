#pragma once

#include <unordered_map>

#include <marocco/graph.h>
#include <marocco/assignment/Mapping.h>

namespace marocco {
namespace placement {

/**
 * External property map which maps Population vertices onto Hardware.
 */
class PlacementMap
{
	typedef graph_t graph_type;
	typedef assignment::Mapping value_type;

	typedef graph_type::vertex_descriptor descriptor_type;
	typedef std::unordered_map<descriptor_type, value_type> container_type;
	// typedef boost::associative_property_map<container_type> adaptor_type;

public:
	bool contains(descriptor_type const& k) const { return mMap.find(k) != mMap.end(); }
	/** @note This used to return a copy instead of a reference
	 *        when distributed property maps were used.
	 */
	value_type const& get(descriptor_type const& k) const { return mMap.at(k); }
	void put(descriptor_type const& k, value_type const& v) { mMap[k] = v; }

private:
	container_type mMap;
};

} // namespace placement
} // namespace marocco
