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
	/**
	 * @brief Construct a new results object by loading mapping results from disk.
	 * @see #load().
	 */
	static Marocco from_file(char const* filename);

	/**
	 * @brief Load mapping results from disk.
	 * @param filename Path to input file.  The extension is used to determine the file
	 *                 format, e.g. `.xml`/`.bin` or `.xml.gz`/`.bin.gz`.
	 * @see #save().
	 */
	void load(char const* filename);

	/**
	 * @brief Save mapping results to disk.
	 * @param filename Path to output file.  The extension is used to determine the file
	 *                 format, e.g. `.xml` or `.bin`.  Optionally a second extension of
	 *                 `.gz` can be added to write results in compressed form,
	 *                 e.g. `results.xml.gz`.
	 */
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
