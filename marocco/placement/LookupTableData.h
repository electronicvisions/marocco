#pragma once

#include "pywrap/compat/hash.hpp"

#include "marocco/graph.h"

#include "hal/Coordinate/HICANN.h"
#include "hal/Coordinate/L1.h"
#include "hal/HICANN/L1Address.h"

namespace marocco {
namespace placement {

/**
 * @brief representation of global pynn neuron
 **/
struct bio_id
{
	// FIXME: we should use the pynn/euter types here
	size_t pop;    //<! population id
	size_t neuron; //<! relative neuron address

	bool operator== (bio_id const& v) const;
	bool operator!= (bio_id const& v) const;
	std::ostream& operator<< (std::ostream& os) const;

	template < typename Archive >
	void serialize(Archive& ar, unsigned int const /*version*/)
	{
		using boost::serialization::make_nvp;
		ar & make_nvp("pop", pop)
		   & make_nvp("neuron", neuron);
	}
};

/**
 * @brief representation of global L1 Address (i.e. HICANN,
 * OutputBufferOnHICANN and the 6-bit address)
 **/
struct hw_id
{
	HMF::Coordinate::HICANNGlobal hicann;
	HMF::Coordinate::NeuronBlockOnHICANN neuron_block;
	// FIXME: Remove addr as it should be query-able by the user.
	HMF::HICANN::L1Address addr;

	bool operator== (hw_id const& k) const;
	bool operator!= (hw_id const& k) const;
	std::ostream& operator<< (std::ostream& os) const;

	template < typename Archive >
	void serialize(Archive& ar, unsigned int const /*version*/)
	{
		using boost::serialization::make_nvp;
		ar & make_nvp("hicann", hicann)
		   & make_nvp("neuron_block", neuron_block)
		   & make_nvp("addr", addr);
	}
};
} // namespace placement
} // namespace marocco


namespace std {
template<>
struct hash<marocco::placement::hw_id>
{
	typedef marocco::placement::hw_id type;
	size_t operator()(type const & t) const;
};

template<>
struct hash<marocco::placement::bio_id>
{
	typedef marocco::placement::bio_id type;
	size_t operator()(type const & t) const;
};
} // namespace std
