#pragma once

#include <array>
#include <tuple>
#include <vector>
#include <boost/serialization/array.h>
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/tuple.h>

#include "hal/Coordinate/HMFGeometry.h"
#include "marocco/placement/OnNeuronBlock.h"

namespace marocco {
namespace placement {

/** This classes combines the OnNeuronBlock mappings of all neuron blocks of a HICANN.
 */
class NeuronBlockMapping
{
public:
	typedef HMF::Coordinate::NeuronBlockOnHICANN  index;
	enum : size_t { size = index::end };

	NeuronBlockMapping();

	OnNeuronBlock&       at(index const& idx);
	OnNeuronBlock const& at(index const& idx) const;

	OnNeuronBlock&       operator[] (index const& idx) noexcept;
	OnNeuronBlock const& operator[] (index const& idx) const noexcept;

	/** Return true if there are neuron assignments or defects marked in any of
	 *  the neuron blocks. */
	bool any() const;

	/** Return the number of available hardware neurons on the HICANN.
	 *  Available neurons are neurons that are neither marked as defect,
	 *  nor have they been assigned already.
	 */
	size_t available() const;

	/** Return the number of available hardware neurons on a given NeuronBlock. */
	size_t available(index const& idx) const;

	/** Return the number of mapped bio neurons on a given NeuronBlock. */
	size_t neurons(index const& idx) const;

private:
	typedef std::array<OnNeuronBlock, size> Mapping;
	Mapping mMapping;

	template<typename Archive>
	void serialize(Archive& ar, unsigned int const /*version*/)
	{
		using boost::serialization::make_nvp;
		ar & make_nvp("neuron_block_mapping", mMapping);
	}
};

} // namespace placement
} // namespace marocco
