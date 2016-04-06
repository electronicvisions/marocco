#pragma once

#include <map>
#include <vector>
#include <boost/variant.hpp>
#include <boost/serialization/export.hpp>

#include "hal/Coordinate/HICANN.h"
#include "hal/Coordinate/Neuron.h"

#include "marocco/placement/parameters/NeuronPlacement.h"

namespace boost {
namespace serialization {
class access;
} // namespace serialization
} // namespace boost

namespace marocco {
namespace placement {
namespace parameters {

class ManualPlacement {
public:
	typedef NeuronPlacement::size_type size_type;
	typedef size_t population_type;

	struct Location
	{
		boost::variant<std::vector<HMF::Coordinate::HICANNOnWafer>,
		               std::vector<HMF::Coordinate::NeuronBlockOnWafer> >
			locations;
		/// Size of hardware neuron.  0 ≙ default neuron size.
		size_type hw_neuron_size;

	private:
		friend class boost::serialization::access;
		template <typename Archive>
		void serialize(Archive& ar, unsigned int const /* version */);
	};

	// TODO: Change to unordered_map, once boost::serialize version is new enough.
	typedef std::map<population_type, Location> mapping_type;

	/**
	 * @brief Request a manual placement of a whole population.
	 * @param pop euter id of population.
	 * @param size Hardware neuron size to use.  0 indicates the default value.
	 * @throw std::invalid_argument If given neuron size is invalid.
	 */
	void on_hicann(
	    population_type pop, HMF::Coordinate::HICANNOnWafer const& hicann, size_type size = 0);
	void on_hicann(
	    population_type pop,
	    std::vector<HMF::Coordinate::HICANNOnWafer> const& hicanns,
	    size_type size = 0);
	void on_neuron_block(
	    population_type pop, HMF::Coordinate::NeuronBlockOnWafer const& block, size_type size = 0);
	void on_neuron_block(
	    population_type pop,
	    std::vector<HMF::Coordinate::NeuronBlockOnWafer> const& blocks,
	    size_type size = 0);
	void with_size(population_type pop, size_type size);

#ifndef PYPLUSPLUS
	mapping_type const& mapping() const;
#endif // !PYPLUSPLUS

private:
#ifndef PYPLUSPLUS
	mapping_type m_mapping;
#endif // !PYPLUSPLUS

	friend class boost::serialization::access;
	template <typename Archive>
	void serialize(Archive& ar, unsigned int const /* version */);
}; // ManualPlacement

} // namespace parameters
} // namespace placement
} // namespace marocco

BOOST_CLASS_EXPORT_KEY(::marocco::placement::parameters::ManualPlacement)
BOOST_CLASS_EXPORT_KEY(::marocco::placement::parameters::ManualPlacement::Location)
