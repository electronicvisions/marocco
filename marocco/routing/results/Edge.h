#pragma once
#include <functional>
#include <boost/serialization/export.hpp>

namespace boost {
namespace serialization {
class access;
} // namespace serialization
} // namespace boost

namespace marocco {
namespace routing {
namespace results {

/**
 * @brief ID of edge in bio graph, used to implement lookup by projection view.
 * @attention This does not correspond to the euter ID of the projection, as projections
 *            are flattened into projection views by marocco.  Each edge in the bio graph
 *            corresponds to one such projection view and has a unique ID.  Thus multiple
 *            edge IDs may correspond to the same euter projection ID.
 */
class Edge
{
public:
	Edge();
	explicit Edge(size_t value);

	size_t value() const;

private:
	friend class boost::serialization::access;
	template <typename Archiver>
	void serialize(Archiver& ar, const unsigned int /* version */);

	size_t m_value;
}; // Edge

bool operator==(Edge const & lhs, Edge const& rhs);
size_t hash_value(Edge const& edge);

} // namespace results
} // namespace routing
} // namespace marocco

namespace std {

template <>
struct hash<marocco::routing::results::Edge>
{
	size_t operator()(marocco::routing::results::Edge const& edge) const
	{
		return hash_value(edge);
	}
};

} // namespace std

BOOST_CLASS_EXPORT_KEY(::marocco::routing::results::Edge)
