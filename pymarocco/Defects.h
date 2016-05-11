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

	PYPP_CLASS_ENUM(Backend) {
		XML,
		None
	};

	Defects();

	/**
	 * @brief Backend for defect data.
	 * Defaults to \c None.
	 */
	Backend backend;

	/**
	 * @brief Path to directory containing defect data.
	 * Marocco will throw during runtime if \c MAROCCO_DEFECTS_PATH is set and this string
	 * is non-empty.
	 */
	std::string path;

	void disable(HMF::Coordinate::HICANNGlobal);

	void inject(HMF::Coordinate::HICANNGlobal,
	            boost::shared_ptr<redman::resources::Hicann>);

#ifndef PYPLUSPLUS
	hicann_map_type const& hicanns() const { return mHicanns; }
#endif // PYPLUSPLUS

private:
	hicann_map_type mHicanns;

	friend class boost::serialization::access;
	template<typename Archive>
	void serialize(Archive& ar, unsigned int const)
	{
		using boost::serialization::make_nvp;
		ar & make_nvp("hicanns", mHicanns);
	}
};

} // pymarocco
