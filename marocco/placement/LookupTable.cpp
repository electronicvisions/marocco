#include "marocco/placement/LookupTable.h"

#include <ostream>
#include <vector>

#include "hal/Coordinate/iter_all.h"
#include "marocco/Logger.h"
#include "marocco/assignment/NeuronBlockSlice.h"
#include "marocco/config.h"
#include "marocco/placement/Result.h"
#include "marocco/util/iterable.h"

namespace marocco {
namespace placement {

LookupTable::LookupTable(Result const &result, resource_manager_t const &mgr, graph_t const &graph)
{
	using namespace HMF::Coordinate;

	// here the mapping between the L1 adresses and the
	// biological Neurons is made
	auto const& om = result.output_mapping;
	for (auto const& h : mgr.allocated())
	{
		for (auto const& outb : iter_all<OutputBufferOnHICANN>())
		{
			auto const& mappings = om.at(h).at(outb);
			for (assignment::AddressMapping const& mapping : mappings) {
				// get a vector of L1Adresses
				auto const& addresses = mapping.addresses();
				// get one population
				auto const& bio = mapping.bio();

				Population const& pop = *graph[bio.population()];

				size_t neuron_index = bio.offset();
				size_t offset = 0;
				for (auto const& address : addresses) {

					MAROCCO_TRACE("RevVal: pop: " << pop.id() << " neuron: " << neuron_index
					                              << " offset: " << offset << ", RevKey: " << h
					                              << " " << outb << " " << address);

					// bio represents one biological neuron
					bio_id const bio{pop.id(), neuron_index + offset};

					// hw represents the corresponding hardware (+ the bio neuron itself)
					hw_id const hw{h, outb, address}; // ECM: the mapping seems to use this for
					                                  // target and source addresses,
					                                  // cf. HICANNTransformator::spike_input()

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

		// sources have no denmem counter part
		if(is_source(vertex, graph)) {
			continue;
		}

		auto const& population = *graph[vertex];
		auto const& mapping = placement.at(vertex);
		// set index of first bio neuron in a population to zero
		size_t bio_neuron_index = 0;

		// each population has a vector with assigned denmem circuits
		for (assignment::NeuronBlockSlice const& nb_slice : mapping) {
			auto denmem_count = nb_slice.size();
			NeuronBlockGlobal neuron_block = nb_slice.coordinate();
			// the 256x2 array of denmem circuits on a HICANN is distributed into
			// 8 blocks of size 32x2
			auto block = neuron_block.toNeuronBlockOnHICANN();
			// this vector points to the beginning of assigned denmem circuits
			// in this terminal
			auto hw_neuron_size = nb_slice.neuron_size(); // size (# of denmems) of logical neuron
			size_t bio_neurons_in_terminal = denmem_count/hw_neuron_size;

			auto offset = nb_slice.offset(); // denmem offset of logical neuron
			auto const neurons_x_per_neuronblock = NeuronOnNeuronBlock::x_type::size;
			auto const neurons_y_per_neuronblock = NeuronOnNeuronBlock::y_type::size;

			// iterate over all bio neurons in this terminal
			for (size_t i = 0; i < bio_neurons_in_terminal; ++i) {
				bio_id bio {population.id(), bio_neuron_index};

				// iterate over the belonging denmem circuits
				for (size_t d = 0; d < hw_neuron_size; ++d) {
					X x{block.value() * neurons_x_per_neuronblock + offset.x() +
					    i * hw_neuron_size / neurons_y_per_neuronblock +
					    d / neurons_y_per_neuronblock};
					Y y{neurons_y_per_neuronblock % 2};
					NeuronOnHICANN point{x, y};
					NeuronGlobal ng{point, neuron_block.toHICANNGlobal()};
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

const LookupTable::hw_to_bio_map_type& LookupTable::getHwToBioMap() const {
	return mHw2BioMap;
}

LookupTable::hw_to_bio_map_type& LookupTable::getHwToBioMap() {
	return mHw2BioMap;
}

const LookupTable::bio_to_hw_map_type& LookupTable::getBioToHwMap() const {
	return mBio2HwMap;
}

LookupTable::bio_to_hw_map_type& LookupTable::getBioToHwMap() {
	return mBio2HwMap;
}

const LookupTable::bio_to_denmem_map_type& LookupTable::getBioToDenmemMap() const {
	return mBio2DenmemMap;
}

LookupTable::bio_to_denmem_map_type& LookupTable::getBioToDenmemMap() {
	return mBio2DenmemMap;
}

} // namespace placement
} // namespace marocco
