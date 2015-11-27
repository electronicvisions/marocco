#pragma once

#include "pywrap/compat/rant.hpp"
#include "hal/Coordinate/Neuron.h"
#include "marocco/config.h"

namespace marocco {
namespace assignment {

/**
 * Instances of `NeuronBlockSlice` are used in the result of the neuron placement to
 * map biological neurons to sets of hardware neurons.
 *
 * `NeuronBlockSlice` is a slice of neurons on a neuron block, defined
 * by the global neuron block coordinate, an offset and the size in
 * number of hardware neurons.
 * Furthermore the neuron_size defines the number of biological neurons mapped
 * onto that slice.
 *
 * @note This is the counterpart of the class NeuronPlacement, which is build up of a
 *       population slice and a hardware_neuron_size.
 */
struct NeuronBlockSlice
{
public:
	typedef HMF::Coordinate::NeuronOnNeuronBlock offset_type;
	typedef HMF::Coordinate::NeuronBlockGlobal coordinate_type;
	typedef std::size_t size_type;

	NeuronBlockSlice(
		coordinate_type coordinate, offset_type offset, size_type size, size_type neuron_size);

	/// Offset, i.e. \c NeuronOnNeuronBlock coordinate of first neuron
	/// in the hardware slice.
	offset_type offset() const;

	/// Number of hardware neurons.
	size_type size() const;

	/// Size (in hardware neurons) of a single bio neuron.
	size_type neuron_size() const;

	/// Number of bio neurons, which are represented by the `size()`
	/// hardware neurons.
	inline size_type bio_size() const
	{
		return size() / neuron_size();
	}

	coordinate_type const& coordinate() const;

private:
	NeuronBlockSlice();

private:
	friend class boost::serialization::access;
	template <typename Archiver>
	void serialize(Archiver& ar, unsigned int const /*version*/)
	{
		ar & mCoordinate;
		ar & mOffset;
		ar & mSize;
		ar & mHwNeuronSize;
	}

	coordinate_type mCoordinate;
	offset_type mOffset;
	size_type mSize;
	size_type mHwNeuronSize;
};

} // namespace assignment
} // namespace marocco
