#pragma once

#include "pywrap/compat/rant.hpp"
#include "hal/Coordinate/HMFGeometry.h"
#include "marocco/config.h"

namespace marocco {
namespace assignment {

/**
 * Instances of `Hardware` are used in the result of the neuron placement to
 * map biological neurons to sets of hardware neurons.
 *
 * `Hardware` actually is a HardwareSlice, i.e. slice of neurons on a neuron block,
 * defined by the global neuron block coordinate, an offset, the size.
 * Furthermore the neuron_size defines the number of biological neurons mapped 
 * onto that slice.
 *
 * @note This is the counterpart of the class NeuronPlacement, which is build up of a 
 *       population slice and a hardware_neuron_size.
 *
 * TODO: in constructor, assure that size is divisible by neuron_size.
 * TODO: rename this class into s.th. more descriptive, e.g. NeuronBlockSlice
 */
struct Hardware
{
public:
	typedef HMF::Coordinate::NeuronOnNeuronBlock offset_type;
	typedef HMF::Coordinate::NeuronBlockGlobal value_type;
	typedef std::size_t size_type;

	Hardware(value_type assign, offset_type offset,
		 size_type size, size_type neuron_size);

	/// returns the number of bio neurons, which are represented by the `size()`
	/// hardware neurons.
	inline size_type bio_size() const
	{
		return size() / mHWNeuronSize;
	}

	/// returns the number of hardware neurons
	size_type size()   const;

	/// returns the offset, i.e. the neuron-on-block coordinate of first neuron
	/// in the hardware slice.
	offset_type offset() const;

	value_type const& get() const;

	/// returns number hardware neuron modules used for one neuron
	size_type hw_neuron_size() const;

	bool operator== (Hardware const& other) const
	{
		if (get() == other.get() && offset() == other.offset()) {
			assert(size() == other.size());
			assert(hw_neuron_size() == other.hw_neuron_size());
			return true;
		}
		return false;
	}

	bool operator!= (Hardware const& other) const
	{
		return !(*this == other);
	}

//private:
	Hardware(); // FIXME: should be private

private:
	friend class boost::serialization::access;
	template<typename Archiver>
	void serialize(Archiver& ar, unsigned int const /*version*/)
	{
		ar & mAssign;
		ar & mOffset;
		ar & mSize;
		ar & mHWNeuronSize;
	}

	value_type mAssign;
	offset_type mOffset;
	size_type mSize;
	size_type mHWNeuronSize;
};

} // namespace assignment
} // namespace marocco
