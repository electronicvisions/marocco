#pragma once

#include <boost/serialization/serialization.hpp>
#include <boost/serialization/nvp.hpp>

#include "marocco/assignment/AddressMapping.h"

namespace marocco {
namespace routing {

/// specifies a pair of a projection and corresponding source ouput buffer assignment
struct HardwareProjection
{
	/// projcetion type
	typedef graph_t::edge_descriptor  projection_type;

	/// source type
	typedef assignment::AddressMapping         source_type;

	source_type const& source() const;
#ifndef PYPLUSPLUS
	source_type&       source();
#endif // PYPLUSPLUS

	projection_type const& projection() const;
#ifndef PYPLUSPLUS
	projection_type&       projection();
#endif // PYPLUSPLUS

	/// returns the number of sources in this Projection 
	size_t size() const;

	HardwareProjection(
		source_type const& source,
		projection_type const& proj);

	bool operator== (HardwareProjection const& rhs) const;

private:
	source_type mSource;
	projection_type mProjection;

	HardwareProjection();

	friend class boost::serialization::access;
	template<typename Archiver>
	void serialize(Archiver& ar, unsigned int const /*version*/);
};


template<typename Archiver>
void HardwareProjection::serialize(Archiver& ar, unsigned int const /*version*/)
{
	using namespace boost::serialization;
	ar & make_nvp("source", mSource)
	   & make_nvp("projection", mProjection);
}

} // namespace routing
} // namespace marocco
