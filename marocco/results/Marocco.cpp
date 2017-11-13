#include "marocco/results/Marocco.h"

#include <unordered_set>

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
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
void Marocco::serialize(Archiver& ar, const unsigned int /* version */)
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

} // namespace results
} // namespace marocco

BOOST_CLASS_EXPORT_IMPLEMENT(::marocco::results::Marocco)

#include "boost/serialization/serialization_helper.tcc"
EXPLICIT_INSTANTIATE_BOOST_SERIALIZE(::marocco::results::Marocco)
