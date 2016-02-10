#include "marocco/placement/LookupTable.h"

#include <ostream>
#include <vector>

#include "hal/Coordinate/iter_all.h"
#include "marocco/Logger.h"
#include "marocco/config.h"
#include "marocco/placement/Result.h"
#include "marocco/util/chunked.h"
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
		for (auto const& dnc : iter_all<DNCMergerOnHICANN>())
		{
			auto const& mappings = om.at(h).at(dnc);
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
					                              << " " << dnc << " " << address);

					// bio represents one biological neuron
					bio_id const bio{pop.id(), neuron_index + offset};

					// FIXME: This assumes 1-to-1 merger tree configuration
					NeuronBlockOnHICANN neuron_block(dnc);


					// hw represents the corresponding hardware (+ the bio neuron itself)
					// ECM: the mapping seems to use this for target and source addresses,
					// cf. HICANNTransformator::spike_input()
					hw_id const hw{h, neuron_block, address};

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
	auto const& onm = result.neuron_placement;
	auto const& placement = onm.placement();

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
		for (NeuronGlobal const& primary_neuron : mapping) {
			NeuronBlockGlobal neuron_block = primary_neuron.toNeuronBlockGlobal();
			placement::OnNeuronBlock const& onb =
			    onm.at(primary_neuron.toHICANNGlobal())[neuron_block.toNeuronBlockOnHICANN()];

			auto it = onb.get(primary_neuron.toNeuronOnNeuronBlock());
			assert(it != onb.end());

	 		size_t const hw_neuron_size = (*it)->neuron_size();
			for (auto& neuron : chunked(onb.neurons(it), hw_neuron_size)) {
				bio_id bio {population.id(), bio_neuron_index};

				for (NeuronOnNeuronBlock nrn : neuron) {
					auto ng = nrn.toNeuronGlobal(neuron_block);
					mBio2DenmemMap[bio].push_back(ng);
					mDenmem2BioMap[ng] = bio;
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

bio_id const&
LookupTable::at(hw_id const& key) const
{
	return mHw2BioMap.at(key);
}

bio_id&
LookupTable::operator[] (HMF::Coordinate::NeuronGlobal const& key)
{
	return mDenmem2BioMap[key];
}

bio_id&
LookupTable::at(HMF::Coordinate::NeuronGlobal const& key)
{
	return mDenmem2BioMap.at(key);
}

bio_id const&
LookupTable::at(HMF::Coordinate::NeuronGlobal const& key) const
{
	return mDenmem2BioMap.at(key);
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

const LookupTable::bio_to_denmem_map_type&
LookupTable::getBioToDenmemMap() const {
	return mBio2DenmemMap;
}

LookupTable::bio_to_denmem_map_type&
LookupTable::getBioToDenmemMap() {
	return mBio2DenmemMap;
}

const LookupTable::denmem_to_bio_map_type&
LookupTable::getDenmemToBioMap() const {
	return mDenmem2BioMap;
}

LookupTable::denmem_to_bio_map_type&
LookupTable::getDenmemToBioMap() {
	return mDenmem2BioMap;
}

} // namespace placement
} // namespace marocco
