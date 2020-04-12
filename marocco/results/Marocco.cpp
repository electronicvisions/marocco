#include <algorithm>

#include "marocco/results/Marocco.h"

#include <unordered_set>

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/graph/adj_list_serialize.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/serialization/nvp.hpp>

#include "halco/common/iter_all.h"

using namespace halco::common;
using namespace halco::hicann::v2;

namespace marocco {
namespace results {

Marocco Marocco::from_file(std::string const& filename)
{
	Marocco result;
	result.load(filename);
	return result;
}

Marocco::Marocco()
{
}

void Marocco::load(std::string const& filename_)
{
	char const* filename = filename_.c_str();
	boost::filesystem::path path(filename);
	if (!boost::filesystem::exists(path)) {
		throw std::runtime_error("file not found");
	}
	boost::filesystem::ifstream ifstream(path);
	boost::iostreams::filtering_stream<boost::iostreams::input> stream;
	if (path.extension() == ".gz") {
		stream.push(boost::iostreams::gzip_decompressor());
		path = path.stem();
	}
	stream.push(ifstream);

	if (path.extension() == ".xml") {
		boost::archive::xml_iarchive{stream} >> boost::serialization::make_nvp("Marocco", *this);
	} else {
		boost::archive::binary_iarchive{stream} >> *this;
	}
}

void Marocco::save(std::string const& filename_, bool overwrite) const
{
	char const* filename = filename_.c_str();
	boost::filesystem::path path(filename);
	if (boost::filesystem::exists(path) && !overwrite) {
		throw std::runtime_error("file already exists");
	}

	boost::filesystem::ofstream ofstream(path);
	boost::iostreams::filtering_stream<boost::iostreams::output> stream;
	if (path.extension() == ".gz") {
		stream.push(boost::iostreams::gzip_compressor());
		path = path.stem();
	}
	stream.push(ofstream);

	if (path.extension() == ".xml") {
		boost::archive::xml_oarchive{stream} << boost::serialization::make_nvp("Marocco", *this);
	} else {
		boost::archive::binary_oarchive{stream} << *this;
	}
}

template <typename Archiver>
void Marocco::serialize(Archiver& ar, const unsigned int version)
{
	using namespace boost::serialization;
	// clang-format off
	ar & make_nvp("resources", resources)
	   & make_nvp("analog_outputs", analog_outputs)
	   & make_nvp("spike_times", spike_times)
	   & make_nvp("placement", placement)
	   & make_nvp("l1_routing", l1_routing)
	   & make_nvp("synapse_routing", synapse_routing);
	// clang-format on
	if (version >= 2) {
		ar& make_nvp("bio_graph", bio_graph);
	}
	if (version >= 3) {
		ar& make_nvp("parameter", parameter);
	}
}

HICANNOnWaferProperties Marocco::properties(halco::hicann::v2::HICANNOnWafer const& hicann) const
{
	// The current implementation is rather expensive... (rapid-prototyping)

	if (!resources.has(hicann)) {
		return {};
	}

	auto it = placement.find(hicann);
	size_t const num_neurons = std::distance(it.begin(), it.end());

	size_t num_injections = 0;
	for (auto const dnc : iter_all<DNCMergerOnHICANN>()) {
		auto it_ = placement.find(DNCMergerOnWafer(dnc, hicann));
		num_injections += std::distance(it_.begin(), it_.end());
	}

	size_t const num_inputs = num_injections - num_neurons;

	std::unordered_set<HLineOnHICANN> horizontal_buses;
	std::unordered_set<VLineOnHICANN> vertical_buses;
	typed_array<size_t, SideHorizontal> num_vertical_buses{{0, 0}};

	// Record used buses on this HICANN
	for (auto const& item : l1_routing) {
		auto const& route = item.route();
		HICANNOnWafer current_hicann;
		for (auto const& segment : route) {
			if (auto const* next_hicann = boost::get<HICANNOnWafer>(&segment)) {
				current_hicann = *next_hicann;
				continue;
			}
			if (current_hicann != hicann) {
				continue;
			}

			if (auto const* hline = boost::get<HLineOnHICANN>(&segment)) {
				horizontal_buses.insert(*hline);
				continue;
			}

			if (auto const* vline = boost::get<VLineOnHICANN>(&segment)) {
				if (vertical_buses.insert(*vline).second) {
					num_vertical_buses[vline->toSideHorizontal()] += 1;
				}
			}
		}
	}

	return {num_neurons, num_inputs, horizontal_buses.size(), num_vertical_buses[left],
			num_vertical_buses[right]};
}

std::vector<L1RouteProperties> Marocco::l1_properties() const
{
	std::vector<L1RouteProperties> l1_route_properties_vec;
	auto const& synapse_routing_synapses = synapse_routing.synapses();

	for (auto const& route_item : l1_routing) {
		auto const& projections = l1_routing.find_projections(route_item);

		std::vector<size_t> projection_ids;
		std::transform(projections.begin(), projections.end(),
		               std::back_inserter(projection_ids),
		               [](auto p) { return p.projection(); });

		auto const& source = placement.find(route_item.source());
		auto const& target = placement.find(route_item.target());
		std::vector<HMF::HICANN::L1Address> l1_addresses;
		std::transform(source.begin(), source.end(), std::back_inserter(l1_addresses),
		               [](auto p) {
			return p.address()->toL1Address();
		});

		auto const& route = route_item.route();
		L1Route route_augmented(route);

		// augment with drivers; primary driver first
		if (auto const* vline = boost::get<VLineOnHICANN>(&route.back())) {

			auto const& synapse_switches = synapse_routing[route_item.target()][*vline];
			for (auto const& item : synapse_switches) {
				auto const& connected_drivers = item.connected_drivers();

				std::vector<SynapseDriverOnHICANN> all_drivers(
				    connected_drivers.drivers().begin(), connected_drivers.drivers().end());

				std::sort(
				    all_drivers.begin(), all_drivers.end(),
				    [](auto const& drv_a, auto const& drv_b) {
					    return drv_a.toEnum() < drv_b.toEnum();
				    });

				// Three cases to be considered:
				// a b c
				//     1
				//   3 2
				// 5 5 5 <- primary driver
				// 7 7
				// 9
				//
				// a: all drivers below primary
				// b: drivers above and below primary
				// c: all drivers above primary
				//
				// there may be more drivers than one or two below and above primary
				//
				// order of drivers to be inserted into L1Route:
				//
				// a: 5, 7, 9
				// b: 5, 3, 5, 7 _or_ 5, 7, 5, 3
				// c: 5, 2, 1
				//
				std::vector<SynapseDriverOnHICANN> drivers_for_augmentation;

				auto const primary = connected_drivers.primary_driver();

				// The datastructure of synapse_routing contains the synapse drivers
				// that shall be used. it does not store the side to which it injects.
				// Thus it has to be deduced if this driver is used for this hicann or the neighbouring one.
				//
				//  Following figure shall help explaining it.
				//  shown are 2 HICANNS with 6 VLines and 6 Synapse drivers.
				//
				//     Driver
				//      v v
				//  |123   456|123   456| << vline
				//  |   1 2   |   1 2   |
				//  |   3 4  x|   3 4   |
				//  |   5 6   |   5 6   |
				//
				// example:  vLine:6 drv:4, will have the same hicann as target, where the route ends.
				// but       vLine 6 drv 3, means that it feeds from the route.target_hicann() into the route_item.target()
				//
				// this gets problematic if `synapse_routing[route_item.target()][*vline]` has two or more entries.
				// this happens if the `route_item.target()` hicann gets feeded from itself by vline 6 drv 4,
				// and from its neighbour with vline 6 drv 3.
				//
				// this is checked here.
				if ((vline->toSideHorizontal() != primary.toSideHorizontal() && route_item.target() == route.target_hicann() ) ||
					(vline->toSideHorizontal() == primary.toSideHorizontal() && route_item.target() != route.target_hicann() )   ) {
					continue;
					/// the SynapseRouting itself does the following:
					/// hicann_ = vline.isLeft() ? hicann.east() : hicann.west();
				}

				bool const check_above_primary = (primary != all_drivers.front());

				// start from primary and go down
				for (auto it = std::find(all_drivers.begin(), all_drivers.end(), primary);
				     it != all_drivers.end(); ++it) {
					drivers_for_augmentation.push_back(*it);
				}

				// if there is anything above the primary driver, start from end and go up
				if (check_above_primary) {
					for (auto it = all_drivers.rbegin(); it != all_drivers.rend(); ++it) {
						drivers_for_augmentation.push_back(*it);
					}
				}

				// erase double countings from going back and forth
				drivers_for_augmentation.erase(
				    std::unique(drivers_for_augmentation.begin(), drivers_for_augmentation.end()),
				    drivers_for_augmentation.end());

				for (auto const& dr : drivers_for_augmentation) {
					route_augmented.append(route_item.target(), dr);
				}

				// augment with synapses to THIS hicann
				for (auto const& proj_item : projections) {
					auto const& synapse_items =
					    synapse_routing_synapses.find(proj_item.projection());
					for (auto const& synapse_item : synapse_items) {
						auto const& syn_source = synapse_item.source_neuron();
						auto const& syn_target = synapse_item.target_neuron();

						if (std::find_if(
						        source.begin(), source.end(),
						        [&syn_source](placement::results::Placement::item_type const& a) {
							        return syn_source == a.bio_neuron();
						        }) == source.end()) {
							continue;
						}
						if (std::find_if(
						        target.begin(), target.end(),
						        [&syn_target](placement::results::Placement::item_type const& a) {
							        return syn_target == a.bio_neuron();
						        }) == target.end()) {
							continue;
						}

						auto const& hw_synapse = synapse_item.hardware_synapse();
						if (hw_synapse.is_initialized()) {
							route_augmented.append(route_item.target(), hw_synapse.get());
						}
					}
				}
			}
		}
		l1_route_properties_vec.push_back({projection_ids, route_augmented, l1_addresses});
	}

	return l1_route_properties_vec;
}

HICANNOnWaferProperties::HICANNOnWaferProperties()
	: m_is_available(false),
	  m_num_neurons(0),
	  m_num_inputs(0),
	  m_num_horizontal_buses(0),
	  m_num_vertical_buses{{0, 0}}
{}

HICANNOnWaferProperties::HICANNOnWaferProperties(
	size_t num_neurons,
	size_t num_inputs,
	size_t num_horizontal_buses,
	size_t num_left_buses,
	size_t num_right_buses)
	: m_is_available(true),
	  m_num_neurons(num_neurons),
	  m_num_inputs(num_inputs),
	  m_num_horizontal_buses(num_horizontal_buses),
	  m_num_vertical_buses{{num_left_buses, num_right_buses}}
{}

bool HICANNOnWaferProperties::is_available() const
{
	return m_is_available;
}

bool HICANNOnWaferProperties::is_transit_only() const
{
	return m_is_available && m_num_neurons == 0 && m_num_inputs == 0;
}

bool HICANNOnWaferProperties::has_neurons() const
{
	return m_num_neurons > 0;
}

bool HICANNOnWaferProperties::has_inputs() const
{
	return m_num_inputs > 0;
}

size_t HICANNOnWaferProperties::num_neurons() const
{
	return m_num_neurons;
}

size_t HICANNOnWaferProperties::num_inputs() const
{
	return m_num_inputs;
}

size_t HICANNOnWaferProperties::num_buses(halco::common::Orientation orientation) const
{
	if (orientation == horizontal) {
		return m_num_horizontal_buses;
	}
	return m_num_vertical_buses[left] + m_num_vertical_buses[right];
}

size_t HICANNOnWaferProperties::num_buses(halco::common::SideHorizontal side) const
{
	return m_num_vertical_buses[side];
}

L1RouteProperties::L1RouteProperties(std::vector<size_t> projection_ids,
                                     const L1Route& route,
                                     const std::vector<HMF::HICANN::L1Address>& l1_addresses)
	: m_projection_ids(projection_ids),
	  m_route(route),
	  m_l1_addresses(l1_addresses)
{
}

std::vector<size_t> L1RouteProperties::projection_ids() const
{
	return m_projection_ids;
}

std::vector<size_t> L1RouteProperties::l1_addresses() const
{
	std::vector<size_t> r;
	for(auto addr : m_l1_addresses) {
		r.push_back(addr.value());
	}
	return r;
}

L1Route L1RouteProperties::route() const
{
	return m_route;
}

} // namespace results
} // namespace marocco

BOOST_CLASS_EXPORT_IMPLEMENT(::marocco::results::Marocco)

#include "boost/serialization/serialization_helper.tcc"
EXPLICIT_INSTANTIATE_BOOST_SERIALIZE(::marocco::results::Marocco)
