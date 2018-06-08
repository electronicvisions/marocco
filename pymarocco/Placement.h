#pragma once

#include <map>
#include <list>
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/list.hpp>
#include "hal/Coordinate/HMFGeometry.h"

namespace pymarocco {

class Placement
{
public:
	typedef size_t PopulationId;
	typedef HMF::Coordinate::HICANNGlobal Index;
	typedef std::list<Index> List;
	typedef std::size_t size_type;
private:
	typedef std::map<PopulationId, std::pair<List, size_type> > mapping_type;

public:
	Placement();

	// place population somewhere, initially we only want to support manually
	// the whole population to keep things simple.
	void add(PopulationId pop, Index const& first);
	void add(PopulationId pop, Index const& first, size_t size);
	void add(PopulationId pop, List list);
	void add(PopulationId pop, List list, size_t size);
	void add(PopulationId pop, size_t size);

	mapping_type::iterator        begin();
	mapping_type::const_iterator  begin() const;
	mapping_type::const_iterator cbegin() const;

	mapping_type::iterator        end();
	mapping_type::const_iterator  end() const;
	mapping_type::const_iterator cend() const;

	/// hack: for python only, because `begin()` and `end()` are not wrapped
	/// properly
	mapping_type const& iter() const;

	void setDefaultNeuronSize(size_type s);
	size_type getDefaultNeuronSize() const;

	/// use smaller capacities for neuron sizes 4 and 8, in order to use less
	/// layer1 resources.
	bool minSPL1;

	/// Reserve OutputBuffer(7) to be used for DNC input and bg events
	bool use_output_buffer7_for_dnc_input_and_bg_hack;
	// (See comments in InputPlacement.cpp)

private:
	mapping_type mPlacement;
	size_type mDefaultNeuronSize;

	friend class boost::serialization::access;
	template<typename Archive>
	void serialize(Archive& ar, unsigned int const)
	{
		using boost::serialization::make_nvp;
		ar & make_nvp("placement", mPlacement)
		   & make_nvp("default_neuron_size", mDefaultNeuronSize)
		   & make_nvp("minSPL1", minSPL1)
		   & make_nvp("use_output_buffer7_for_dnc_input_and_bg_hack",
		              use_output_buffer7_for_dnc_input_and_bg_hack);
	}
};

} // pymarocco
