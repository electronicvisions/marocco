#pragma once

#include <set>
#include <boost/serialization/export.hpp>
#include <boost/unordered_map.hpp>

#include "halco/hicann/v2/hicann.h"
#include "halco/hicann/v2/external.h"
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
	typedef halco::hicann::v2::FPGAOnWafer fpga_type;
	typedef halco::hicann::v2::HighspeedLinkOnWafer hslink_type;

public:

	void add(hicann_type const& hicann);
	bool has(hicann_type const& hicann) const;

	void add(fpga_type const& fpga);
	bool has(fpga_type const& fpga) const;

	void add(hslink_type const& hslink);
	bool has(hslink_type const& hslink) const;

private:
	std::set<hicann_type> m_available_hicanns;
	std::set<fpga_type> m_available_fpgas;
	std::set<hslink_type> m_available_hslinks;

	friend class boost::serialization::access;
	template <typename Archiver>
	void serialize(Archiver& ar, const unsigned int /* version */);
}; // Resources

} // namespace results
} // namespace marocco

BOOST_CLASS_EXPORT_KEY(::marocco::results::Resources)
