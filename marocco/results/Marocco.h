#pragma once

#include <boost/serialization/export.hpp>

#include "halco/hicann/v2/hicann.h"
#include "halco/common/relations.h"
#include "halco/common/typed_array.h"

#include "marocco/BioGraph.h"
#include "marocco/coordinates/L1Route.h"
#include "marocco/parameter/results/AnalogOutputs.h"
#include "marocco/parameter/results/SpikeTimes.h"
#include "marocco/placement/results/Placement.h"
#include "marocco/results/Resources.h"
#include "marocco/routing/results/L1Routing.h"
#include "marocco/routing/results/SynapseRouting.h"
#include "marocco/parameter/results/Parameter.h"

#include "hal/HICANN/L1Address.h"

namespace boost {
namespace serialization {
class access;
} // namespace serialization
} // namespace boost

namespace marocco {
namespace results {

class HICANNOnWaferProperties;
class L1RouteProperties;

/**
 * @brief Container used to store mapping results.
 */
class Marocco {
public:
	/**
	 * @brief Construct a new results object by loading mapping results from disk.
	 * @see #load().
	 */
	static Marocco from_file(std::string const& filename);

	Marocco();

	/**
	 * @brief Load mapping results from disk.
	 * @param filename Path to input file.  The extension is used to determine the file
	 *                 format, e.g. `.xml`/`.bin` or `.xml.gz`/`.bin.gz`.
	 * @see #save().
	 */
	void load(std::string const& filename);

	/**
	 * @brief Save mapping results to disk.
	 * @param filename Path to output file.  The extension is used to determine the file
	 *                 format, e.g. `.xml` or `.bin`.  Optionally a second extension of
	 *                 `.gz` can be added to write results in compressed form,
	 *                 e.g. `results.xml.gz`.
	 */
	void save(std::string const& filename, bool overwrite = false) const;

	Resources resources;
	parameter::results::AnalogOutputs analog_outputs;
	parameter::results::SpikeTimes spike_times;
	parameter::results::Parameter parameter;
	placement::results::Placement placement;
	routing::results::L1Routing l1_routing;
	routing::results::SynapseRouting synapse_routing;
	BioGraph::graph_type bio_graph;

	/**
	 * @brief Create an object representing overview properties of a single HICANN.
	 * @param h Coordinate of the HICANN
	 */
	HICANNOnWaferProperties properties(halco::hicann::v2::HICANNOnWafer const& hicann) const;

	/**
	 * @brief Create an object containing all L1 routes.
	 */
	std::vector<L1RouteProperties> l1_properties() const;

private:
	friend class boost::serialization::access;
	template <typename Archiver>
	void serialize(Archiver& ar, const unsigned int version);
}; // Marocco

class HICANNOnWaferProperties
{
public:
	HICANNOnWaferProperties();
	HICANNOnWaferProperties(
		size_t num_neurons,
		size_t num_inputs,
		size_t num_horizontal_buses,
		size_t num_left_buses,
		size_t num_right_buses);

	bool is_available() const;
	bool is_transit_only() const;
	bool has_neurons() const;
	bool has_inputs() const;
	size_t num_neurons() const;
	size_t num_inputs() const;
	size_t num_buses(halco::common::Orientation orientation) const;
	size_t num_buses(halco::common::SideHorizontal side) const;

private:
	bool m_is_available;
	size_t m_num_neurons;
	size_t m_num_inputs;
	size_t m_num_horizontal_buses;
	halco::common::typed_array<size_t, halco::common::SideHorizontal> m_num_vertical_buses;
}; // HICANNOnWaferProperties

class L1RouteProperties
{
public:
	L1RouteProperties(std::vector<size_t> projection_ids, const L1Route& route,
	                  const std::vector<HMF::HICANN::L1Address>& l1_addresses);

	std::vector<size_t> projection_ids() const;
	L1Route route() const;
	std::vector<size_t> l1_addresses() const;

private:
	std::vector<size_t> m_projection_ids;
	L1Route m_route;
	std::vector<HMF::HICANN::L1Address> m_l1_addresses;
};

} // namespace results
} // namespace marocco

BOOST_CLASS_EXPORT_KEY(::marocco::results::Marocco)
BOOST_CLASS_VERSION(::marocco::results::Marocco, 3)
