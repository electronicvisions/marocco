#include "marocco/results/Resources.h"

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/set.hpp>
#include <boost/serialization/boost_unordered_map.hpp>

namespace marocco {
namespace results {

void Resources::add(hicann_type const& hicann)
{
	m_available_hicanns.insert(hicann);
}

bool Resources::has(hicann_type const& hicann) const
{
	return m_available_hicanns.find(hicann) != m_available_hicanns.end();
}

void Resources::add(fpga_type const& fpga)
{
	m_available_fpgas.insert(fpga);
}

bool Resources::has(fpga_type const& fpga) const
{
	return m_available_fpgas.find(fpga) != m_available_fpgas.end();
}

void Resources::add(hslink_type const& hslink)
{
	m_available_hslinks.insert(hslink);
}

bool Resources::has(hslink_type const& hslink) const
{
	return m_available_hslinks.find(hslink) != m_available_hslinks.end();
}

template <typename Archiver>
void Resources::serialize(Archiver& ar, const unsigned int /* version */)
{
	using namespace boost::serialization;
	// clang-format off
	ar & make_nvp("available_hicanns", m_available_hicanns)
	   & make_nvp("available_fpgas", m_available_fpgas)
	   & make_nvp("available_hslinks", m_available_hslinks);
	// clang-format on
}

} // namespace results
} // namespace marocco

BOOST_CLASS_EXPORT_IMPLEMENT(::marocco::results::Resources)

#include "boost/serialization/serialization_helper.tcc"
EXPLICIT_INSTANTIATE_BOOST_SERIALIZE(::marocco::results::Resources)
