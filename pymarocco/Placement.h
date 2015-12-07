#pragma once

#include <map>
#include <list>
#include <vector>
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/list.hpp>
#include <boost/variant.hpp>
#include "hal/Coordinate/HMFGeometry.h"

namespace pymarocco {

class Placement
{
public:
	typedef size_t PopulationId;
	typedef std::list<HMF::Coordinate::HICANNGlobal> List;
	typedef std::size_t size_type;

	static size_type const defaultNeuronSize;
	static size_type const maxNeuronSize;

	PYPP_INSTANTIATE(List);

private:
#ifndef PYPLUSPLUS
	struct Location
	{
		boost::variant<std::vector<HMF::Coordinate::HICANNGlobal>,
		               std::vector<HMF::Coordinate::NeuronBlockGlobal> > locations;
		/// Size of hardware neuron.  0 ≙ default neuron size.
		// FIXME: this property makes no sense for spike input
		size_type hw_neuron_size;

	private:
		friend class boost::serialization::access;
		template <typename Archive>
		void serialize(Archive& ar, unsigned int const)
		{
			using boost::serialization::make_nvp;
			ar& make_nvp("locations", locations);
			ar& make_nvp("hw_neuron_size", hw_neuron_size);
		}
	};

	typedef std::map<PopulationId, Location> mapping_type;
#endif // !PYPLUSPLUS

public:
	Placement();

	/** Request a manual placement of a whole population.
	 *  @param size Hardware neuron size to use.  0 indicates the default value.
	 *  @note Any remaining unplaced chunks of the population will
	 *        undergo automatic placement.
	 */
	void add(PopulationId pop, HMF::Coordinate::HICANNGlobal const& hicann, size_type size = 0);
	void add(PopulationId pop, List const& hicanns, size_type size = 0);
	void add(PopulationId pop, size_type size);

#ifndef PYPLUSPLUS
	mapping_type::iterator        begin();
	mapping_type::const_iterator  begin() const;
	mapping_type::const_iterator cbegin() const;

	mapping_type::iterator        end();
	mapping_type::const_iterator  end() const;
	mapping_type::const_iterator cend() const;

	mapping_type::iterator find(mapping_type::key_type const& key);
	mapping_type::const_iterator find(mapping_type::key_type const& key) const;
#endif // !PYPLUSPLUS

	void setDefaultNeuronSize(size_type s);
	size_type getDefaultNeuronSize() const;

	/**
	 * @brief Place less neurons per neuron block, in order to use less L1 resources.
	 * @note This only works when automatic placement is used and applies to neuron sizes
	 *       2, 4 and 8.
	 */
	bool minSPL1;

	/// Reserve OutputBuffer(7) to be used for DNC input and bg events
	bool use_output_buffer7_for_dnc_input_and_bg_hack;
	// (See comments in InputPlacement.cpp)

private:
	/// checks if neuron size is a multiple of 2 and not larger than 64.
	/// throws runtime_error otherwise.
	void checkNeuronSize(size_type size);

#ifndef PYPLUSPLUS
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
#endif // !PYPLUSPLUS
};

} // pymarocco
