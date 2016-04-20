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

	auto const& neuron_placement = result.neuron_placement;

	for (auto const& item : neuron_placement) {
		auto const& address = item.address();
		if (address == boost::none) {
			continue;
		}

		bio_id const bio{graph[item.population()]->id(), item.neuron_index()};
		hw_id const hw{address->toHICANNOnWafer(), address->toDNCMergerOnHICANN(),
		               address->toL1Address()};

		mHw2BioMap[hw] = bio;
		mBio2HwMap[bio].push_back(hw);
	}

	// here the mapping between the addresses of the denmem circuits
	// and the biological neurons is made

	// loop over all populations wrapped as graph vertex type
	for (auto const& vertex : make_iterable(boost::vertices(graph))) {

		// sources have no denmem counter part
		if(is_source(vertex, graph)) {
			continue;
		}

		auto const& population = *graph[vertex];

		for (auto const& item : neuron_placement.find(vertex)) {
			bio_id bio {population.id(), item.neuron_index()};

			for (NeuronOnWafer denmem : item.logical_neuron()) {
				mBio2DenmemMap[bio].push_back(denmem);
				mDenmem2BioMap[denmem] = bio;
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

bio_id& LookupTable::operator[](HMF::Coordinate::NeuronOnWafer const& key)
{
	return mDenmem2BioMap[key];
}

bio_id& LookupTable::at(HMF::Coordinate::NeuronOnWafer const& key)
{
	return mDenmem2BioMap.at(key);
}

bio_id const& LookupTable::at(HMF::Coordinate::NeuronOnWafer const& key) const
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
