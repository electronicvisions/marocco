#include "marocco/routing/results/Edge.h"

#include <boost/serialization/nvp.hpp>

namespace marocco {
namespace routing {
namespace results {

Edge::Edge()
	: m_value(0u)
{
}

Edge::Edge(size_t value)
	: m_value(value)
{
}

size_t Edge::value() const
{
	return m_value;
}

template <typename Archiver>
void Edge::serialize(Archiver& ar, const unsigned int /* version */)
{
	using namespace boost::serialization;
	ar & make_nvp("value", m_value);
}

bool operator==(Edge const & lhs, Edge const& rhs)
{
	return lhs.value() == rhs.value();
}

size_t hash_value(Edge const& edge)
{
	return edge.value();
}

} // namespace results
} // namespace routing
} // namespace marocco

BOOST_CLASS_EXPORT_IMPLEMENT(::marocco::routing::results::Edge)

#include "boost/serialization/serialization_helper.tcc"
EXPLICIT_INSTANTIATE_BOOST_SERIALIZE(::marocco::routing::results::Edge)
