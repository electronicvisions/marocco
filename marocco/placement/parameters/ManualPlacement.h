#pragma once

#ifndef PYPLUSPLUS
#include <unordered_map>
#endif // !PYPLUSPLUS
#include <vector>

#include <boost/serialization/export.hpp>
#include <boost/variant.hpp>

#ifndef PYPLUSPLUS
#include "boost/serialization/unordered_map.h"
#endif // !PYPLUSPLUS

#include "hal/Coordinate/HICANN.h"
#include "hal/Coordinate/Neuron.h"

#include "marocco/coordinates/LogicalNeuron.h"
#include "marocco/placement/parameters/NeuronPlacement.h"
#include "marocco/assignment/PopulationSlice.h"

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
	typedef assignment::PopulationSlice::population_type population_type;
	typedef assignment::PopulationSlice::mask_element_type mask_element_type;
	typedef assignment::PopulationSlice::mask_type mask_type;

	struct Location
	{
		boost::variant<std::vector<HMF::Coordinate::HICANNOnWafer>,
		               std::vector<HMF::Coordinate::NeuronBlockOnWafer>,
		               std::vector<LogicalNeuron> >
			locations;
		/// Size of hardware neuron.  0 ≙ default neuron size.
		size_type hw_neuron_size;
		mask_type mask;

	private:
		friend class boost::serialization::access;
		template <typename Archive>
		void serialize(Archive& ar, unsigned int const /* version */);
	};

#ifndef PYPLUSPLUS
	// mapping_type stores the placement informations of all manually placed Populations.
	// Since Populations are also partially placed on different locations using PopulationViews
	// each Population (represented by its Id of type population_type) is assigned to a vector
	// containing placement information for each PopulationView.
	// The PopulationView (represented by the value mask) lists the used neurons of the current
	// Population.
	typedef std::unordered_map<population_type, std::vector<Location>> mapping_type;
#endif // !PYPLUSPLUS

	/**
	 * @brief Request a manual placement of a whole population or population view.
	 * @param pop euter id of population.
	 * @param mask mask of population view (full mask for population).
	 * @param size Hardware neuron size to use.  0 indicates the default value.
	 * @throw std::invalid_argument If given neuron size is invalid.
	 */
	void on_hicann(
	    population_type pop,
	    mask_type mask,
	    HMF::Coordinate::HICANNOnWafer const& hicann,
	    size_type size = 0);
	void on_hicann(
	    population_type pop,
	    mask_type mask,
	    std::vector<HMF::Coordinate::HICANNOnWafer> const& hicanns,
	    size_type size = 0);
	void on_neuron_block(
	    population_type pop,
	    mask_type mask,
	    HMF::Coordinate::NeuronBlockOnWafer const& block,
	    size_type size = 0);
	void on_neuron_block(
	    population_type pop,
	    mask_type mask,
	    std::vector<HMF::Coordinate::NeuronBlockOnWafer> const& blocks,
	    size_type size = 0);
	void on_neuron(population_type pop, mask_type mask, LogicalNeuron const& neuron);
	void on_neuron(population_type pop, mask_type mask, std::vector<LogicalNeuron> const& neurons);
	void with_size(population_type pop, mask_type mask, size_type size);

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
