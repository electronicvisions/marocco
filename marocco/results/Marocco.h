#pragma once

#include <boost/serialization/export.hpp>

#include "halco/hicann/v2/hicann.h"
#include "halco/common/relations.h"
#include "halco/common/typed_array.h"

#include "marocco/parameter/results/AnalogOutputs.h"
#include "marocco/parameter/results/SpikeTimes.h"
#include "marocco/placement/results/Placement.h"
#include "marocco/results/Resources.h"
#include "marocco/routing/results/L1Routing.h"
#include "marocco/routing/results/SynapseRouting.h"

namespace boost {
namespace serialization {
class access;
} // namespace serialization
} // namespace boost

namespace marocco {
namespace results {

class HICANNOnWaferProperties;

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

	Resources resources;
	parameter::results::AnalogOutputs analog_outputs;
	parameter::results::SpikeTimes spike_times;
	placement::results::Placement placement;
	routing::results::L1Routing l1_routing;
	routing::results::SynapseRouting synapse_routing;

	/**
	 * @brief Create an object representing overview properties of a single HICANN.
	 * @param h Coordinate of the HICANN
	 */
	HICANNOnWaferProperties properties(halco::hicann::v2::HICANNOnWafer const& hicann) const;

private:
	friend class boost::serialization::access;
	template <typename Archiver>
	void serialize(Archiver& ar, const unsigned int /* version */);
}; // Marocco

class HICANNOnWaferProperties
{
public:
	HICANNOnWaferProperties();
	HICANNOnWaferProperties(size_t num_neurons, size_t num_inputs);

	bool is_available() const;
	bool is_transit_only() const;
	bool has_neurons() const;
	bool has_inputs() const;
	size_t num_neurons() const;
	size_t num_inputs() const;

private:
	bool m_is_available;
	size_t m_num_neurons;
	size_t m_num_inputs;
}; // HICANNOnWaferProperties

} // namespace results
} // namespace marocco

BOOST_CLASS_EXPORT_KEY(::marocco::results::Marocco)
