#pragma once

#include <map>

#include <boost/shared_ptr.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/shared_ptr.hpp>

#include "hal/Coordinate/HMFGeometry.h"
#include "redman/resources/Hicann.h"

namespace pymarocco {

class Defects {
public:
	typedef std::map<HMF::Coordinate::HICANNGlobal, boost::shared_ptr<redman::resources::Hicann> >
	hicann_map_type;

	Defects();

	float getNeuronDefect() const;
	void setNeuronDefect(float v);

	float getBusDefect() const;
	void setBusDefect(float v);

	void inject(HMF::Coordinate::HICANNGlobal,
	            boost::shared_ptr<redman::resources::Hicann>);

#ifndef PYPLUSPLUS
	hicann_map_type const& hicanns() const { return mHicanns; }
#endif // PYPLUSPLUS

private:
	hicann_map_type mHicanns;
	float mNeuronDefects;
	float mBusDefects;

	friend class boost::serialization::access;
	template<typename Archive>
	void serialize(Archive& ar, unsigned int const)
	{
		using boost::serialization::make_nvp;
		ar & make_nvp("hicanns", mHicanns)
		   & make_nvp("neuron_defect", mNeuronDefects)
		   & make_nvp("bus_defect", mBusDefects);
	}
};

} // pymarocco
