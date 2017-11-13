#pragma once

#include <set>
#include <boost/serialization/export.hpp>

#include "halco/hicann/v2/hicann.h"
#include "marocco/util/iterable.h"

namespace boost {
namespace serialization {
class access;
} // namespace serialization
} // namespace boost

namespace marocco {
namespace results {

/**
 * @brief Container used to record available resources.
 */
class Resources
{
	typedef halco::hicann::v2::HICANNOnWafer hicann_type;

public:
	void add(hicann_type const& hicann);
	bool has(hicann_type const& hicann) const;

private:
	std::set<hicann_type> m_available_hicanns;

	friend class boost::serialization::access;
	template <typename Archiver>
	void serialize(Archiver& ar, const unsigned int /* version */);
}; // Resources

} // namespace results
} // namespace marocco

BOOST_CLASS_EXPORT_KEY(::marocco::results::Resources)
