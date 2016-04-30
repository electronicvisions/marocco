#pragma once

#include <boost/serialization/export.hpp>

#include "marocco/placement/results/Placement.h"

namespace boost {
namespace serialization {
class access;
} // namespace serialization
} // namespace boost

namespace marocco {
namespace results {

/**
 * @brief Container used to store mapping results.
 */
class Marocco {
public:
	static Marocco from_file(char const* filename);
	void save(char const* filename, bool overwrite = false) const;

	placement::results::Placement placement;

private:
	friend class boost::serialization::access;
	template <typename Archiver>
	void serialize(Archiver& ar, const unsigned int /* version */);
}; // Marocco

} // namespace results
} // namespace marocco

BOOST_CLASS_EXPORT_KEY(::marocco::results::Marocco)
