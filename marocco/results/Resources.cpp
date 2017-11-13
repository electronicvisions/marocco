#include "marocco/results/Resources.h"

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/set.hpp>

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

template <typename Archiver>
void Resources::serialize(Archiver& ar, const unsigned int /* version */)
{
	using namespace boost::serialization;
	// clang-format off
	ar & make_nvp("available_hicanns", m_available_hicanns);
	// clang-format on
}

} // namespace results
} // namespace marocco

BOOST_CLASS_EXPORT_IMPLEMENT(::marocco::results::Resources)

#include "boost/serialization/serialization_helper.tcc"
EXPLICIT_INSTANTIATE_BOOST_SERIALIZE(::marocco::results::Resources)
