#pragma once

/**
 * Reverse Mapping
 *
 * For the interpretation of hardware results we need the mapping in reverse
 * order, meaning from hardware value to pynn value.
 * Therefore, we need to generate this mapping at the time of forward mapping.
 * And propagate it to the reverse transformation instance.
 *
 **/

#include <unordered_map>
#include <vector>
#include <iosfwd>
#include <set>

#include "marocco/graph.h"
#include "marocco/resource/HICANNManager.h"
#include "marocco/placement/LookupTableData.h"


// fwd decls
namespace marocco {
namespace resource {
class HICANNManager;
}
typedef marocco::resource::HICANNManager resource_manager_t;

namespace placement {
class Result; // fwd dcl

/**
 * @class LookupTable
 *
 * @brief contains the actual reverse mapping.
 **/
class LookupTable
{
public:

	typedef std::unordered_map< hw_id, bio_id > hw_to_bio_map_type;
	typedef std::unordered_map<HMF::Coordinate::NeuronOnWafer, bio_id> denmem_to_bio_map_type;
	typedef std::unordered_map< bio_id, std::vector< hw_id > > bio_to_hw_map_type;
	typedef std::unordered_map<bio_id, std::vector<HMF::Coordinate::NeuronOnWafer> >
		bio_to_denmem_map_type;

	LookupTable() = default;
	LookupTable(LookupTable const&) = default;
	LookupTable(LookupTable&&) = default;
	LookupTable(Result const &result, resource_manager_t const &mgr, graph_t const &graph);

	// hw to bio transformation
	bio_id&       operator[] (hw_id const& key);
	bio_id&       at(hw_id const& key);
	bio_id const& at(hw_id const& key) const;
	bio_id& operator[](HMF::Coordinate::NeuronOnWafer const& key);
	bio_id& at(HMF::Coordinate::NeuronOnWafer const& key);
	bio_id const& at(HMF::Coordinate::NeuronOnWafer const& key) const;

	// bio to hw transformation
	std::vector< hw_id > &operator[](bio_id const &key);
	std::vector< hw_id > &at(bio_id const &key);
	std::vector< hw_id > const &at(bio_id const &key) const;

	const hw_to_bio_map_type& getHwToBioMap() const;
	hw_to_bio_map_type&       getHwToBioMap();

	const bio_to_hw_map_type& getBioToHwMap() const;
	bio_to_hw_map_type&       getBioToHwMap();

	const bio_to_denmem_map_type& getBioToDenmemMap() const;
	bio_to_denmem_map_type&       getBioToDenmemMap();

	const denmem_to_bio_map_type& getDenmemToBioMap() const;
	denmem_to_bio_map_type&       getDenmemToBioMap();

	size_t size() const;
	bool empty() const;

private:

	hw_to_bio_map_type mHw2BioMap;
	denmem_to_bio_map_type mDenmem2BioMap;
	bio_to_hw_map_type mBio2HwMap;
	bio_to_denmem_map_type mBio2DenmemMap;

	friend class boost::serialization::access;
	template < typename Archive >
	void serialize(Archive &ar, unsigned int const /*version*/)
	{
		using boost::serialization::make_nvp;
		// ECM: boost::serialization does not yet support serialization of
		// std::unordered_map; to workaround this problem, we de-serialize
		// from/to a vector of key-value pairs.
		std::vector< std::pair< hw_to_bio_map_type::key_type, hw_to_bio_map_type::mapped_type > >
		    mHw2BioMapAsVector;
		std::vector< std::pair< bio_to_hw_map_type::key_type, bio_to_hw_map_type::mapped_type > >
		    mBio2HwMapAsVector;
		std::vector< std::pair< bio_to_denmem_map_type::key_type,
		                        bio_to_denmem_map_type::mapped_type > > mBio2DenmemMapAsVector;
		if (Archive::is_saving::value) {
			std::copy(mHw2BioMap.begin(), mHw2BioMap.end(), std::back_inserter(mHw2BioMapAsVector));
			std::copy(mBio2HwMap.begin(), mBio2HwMap.end(), std::back_inserter(mBio2HwMapAsVector));
			std::copy(mBio2DenmemMap.begin(), mBio2DenmemMap.end(),
			          std::back_inserter(mBio2DenmemMapAsVector));
		}
		ar & make_nvp("mHw2BioMapAsVector", mHw2BioMapAsVector);
		ar & make_nvp("mBio2HwMapAsVector", mBio2HwMapAsVector);
		ar & make_nvp("mBio2DenmemMapAsVector", mBio2DenmemMapAsVector);
		if (Archive::is_loading::value) {
			std::copy(mHw2BioMapAsVector.begin(), mHw2BioMapAsVector.end(),
			          std::inserter(mHw2BioMap, mHw2BioMap.end()));
			std::copy(mBio2HwMapAsVector.begin(), mBio2HwMapAsVector.end(),
			          std::inserter(mBio2HwMap, mBio2HwMap.end()));
			std::copy(mBio2DenmemMapAsVector.begin(), mBio2DenmemMapAsVector.end(),
			          std::inserter(mBio2DenmemMap, mBio2DenmemMap.end()));
		}
	}
};

} // namespace placement
} // namespace marocco
