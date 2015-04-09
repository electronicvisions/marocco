#include "marocco/placement/LookupTable.h"

#include <ostream>
#include <vector>

#include "marocco/placement/Result.h"
#include "hal/Coordinate/iter_all.h"
#include "marocco/assignment/Hardware.h"
#include "marocco/config.h"

namespace marocco {
namespace placement {

bool hw_id::operator==(hw_id const &k) const
{
	return hicann == k.hicann && outb == k.outb && addr == k.addr;
}

bool hw_id::operator!=(hw_id const &k) const
{
	return !(*this == k);
}

std::ostream& hw_id::operator<< (std::ostream& os) const
{
	return os << hicann << " " << outb << " " << addr;
}

bool bio_id::operator==(bio_id const &v) const
{
	return pop == v.pop && neuron == v.neuron;
}

bool bio_id::operator!=(bio_id const &v) const
{
	return !(*this == v);
}

std::ostream& bio_id::operator<< (std::ostream& os) const
{
	return os << pop << " " << neuron;
}

LookupTable::LookupTable(Result const &result, resource_manager_t const &mgr, graph_t const &graph)
{
	using namespace HMF::Coordinate;
	auto const& popmap = boost::get(population_t(), graph);

	// here the mapping between the L1 adresses and the
	// biological Neurons is made
	auto const& om = result.output_mapping;
	for (auto const& h : mgr.allocated())
	{
		for (auto const& outb : iter_all<OutputBufferOnHICANN>())
		{
			auto const& mappings = om.at(h).at(outb);
			for (assignment::AddressMapping const& am : mappings)
			{
				// get a vector of L1Adresses
				auto const& addresses = am.addresses();
				// get one population
				auto const& bio = am.bio();

				Population const& pop = *popmap[bio.population()];

				size_t neuron_index = bio.offset();
				size_t offset = 0;
				for (auto const& address : addresses) {

					MAROCCO_TRACE("RevVal: pop: " << pop.id() << " neuron: " << neuron_index
					                              << " offset: " << offset << ", RevKey: " << h
					                              << " " << outb << " " << address);

					hw_id const hw {h, outb, address};
					// now variable bio represents one biological neuron
					bio_id const bio {pop.id(), neuron_index + offset};

					// one denmem circuit ("hardware neuron") belongs
					// to one PyNN neuron, but one PyNN neuron belongs
					// to one or more denmem circuits
					mHw2BioMap[hw] = bio;
					mBio2HwMap[bio].push_back(hw);
					++offset;
				}
			}
		}
	}

	// here the mapping between the addresses of the denmem circuits
	// and the biological neurons is made
	auto const& onm = result.neuron_placement; // get output neuron mapping result
	auto const& placement = onm.placement(); // get the distributed placement map

	// loop over all populations wrapped as graph vertex type
	for (auto const& vertex : make_iterable(boost::vertices(graph))) {
		auto const& population = *popmap[vertex];
		auto const& mapping = placement.get(vertex);

		std::vector<assignment::Hardware> const& am = mapping.assignment();
		// set index of first bio neuron in a population to zero
		size_t bio_neuron_index = 0;

		// each population has a vector with assigned denmem circuits
		// wrapped in marocco/assignment/Hardware.h
		for (std::vector<assignment::Hardware>::const_iterator hardware = am.begin(); hardware != am.end(); ++hardware) {
			auto denmem_count = hardware->size();
			auto const& terminal_proxy = hardware->get();
			// the 256x2 array of denmem circuits on a HICANN is distributed into
			// 8 blocks of size 32x2
			auto block = terminal_proxy.toNeuronBlockOnHICANN();
			// this vector points to the beginning of assigned denmem circuits
			// in this terminal
			auto hw_neuron_size = hardware->hw_neuron_size(); // size (# of denmems) of logical neuron
			size_t bio_neurons_in_terminal = denmem_count/hw_neuron_size;

			auto offset = hardware->offset(); // denmem offset of logical neuron
			auto const neurons_x_per_neuronblock = HMF::Coordinate::NeuronOnNeuronBlock::x_type::size;
			auto const neurons_y_per_neuronblock = HMF::Coordinate::NeuronOnNeuronBlock::y_type::size;

			// iterate over all bio neurons in this terminal
			for (size_t i = 0; i < bio_neurons_in_terminal; ++i) {
				bio_id bio {population.id(), bio_neuron_index};

				// iterate over the belonging denmem circuits
				for (size_t d = 0; d < hw_neuron_size; ++d) {
					geometry::X x{block.value() * neurons_x_per_neuronblock + offset.x() +
					              i * hw_neuron_size / neurons_y_per_neuronblock +
					              d / neurons_y_per_neuronblock};
					geometry::Y y{neurons_y_per_neuronblock % 2};
					HMF::Coordinate::NeuronOnHICANN point{x, y};
					HMF::Coordinate::NeuronGlobal ng {point, terminal_proxy.toHICANNGlobal()};
					// now save bio_id to NeuronGlobal mapping
					mBio2DenmemMap[bio].push_back(ng);
				}
				++bio_neuron_index;
			}
		}
	}
}

// access the hardware 2 bio coordinate transformation
bio_id&
LookupTable::operator[] (hw_id const& key)
{
	return mHw2BioMap[key];
}

bio_id&
LookupTable::at(hw_id const& key)
{
	return mHw2BioMap.at(key);
}

bio_id
const& LookupTable::at(hw_id const& key) const
{
	return mHw2BioMap.at(key);
}

size_t LookupTable::size() const
{
	return mHw2BioMap.size() + mBio2HwMap.size();
}

bool LookupTable::empty() const
{
	return mHw2BioMap.empty() && mBio2HwMap.empty();
}

// access the bio 2 hardware coordinate transformation
std::vector<hw_id>&
LookupTable::operator[] (bio_id const& key)
{
	return mBio2HwMap[key];
}

std::vector<hw_id>&
LookupTable::at(bio_id const& key)
{
	return mBio2HwMap.at(key);
}

std::vector<hw_id>
const& LookupTable::at(bio_id const& key) const
{
	return mBio2HwMap.at(key);
}

} // namespace placement
} // namespace marocco

namespace std {

size_t hash<marocco::placement::hw_id>::operator()(marocco::placement::hw_id const & t) const
{
	size_t hash = hash_value(t.hicann);
	boost::hash_combine(hash, hash_value(t.outb));
	boost::hash_combine(hash, hash_value(t.addr));
	return hash;
}

size_t hash<marocco::placement::bio_id>::operator()(marocco::placement::bio_id const & t) const
{
	size_t hash = hash_value(t.pop);
	boost::hash_combine(hash, hash_value(t.neuron));
	return hash;
}
} // namespace std
