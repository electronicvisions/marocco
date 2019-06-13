#pragma once

#include <map>

#include <boost/shared_ptr.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/version.hpp>

#include "hal/Coordinate/HMFGeometry.h"
#include "redman/resources/Hicann.h"
#include "redman/resources/Wafer.h"

namespace pymarocco {

class Defects {
public:
	typedef boost::shared_ptr<redman::resources::Wafer> defects_t;
	typedef boost::shared_ptr<redman::resources::Wafer const> defects_const_t;

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

	void set(defects_t);
	defects_t wafer();
	defects_const_t wafer() const;

private:
	defects_t mWafer;

	friend class boost::serialization::access;
	template<typename Archive>
	void serialize(Archive& ar, unsigned int const version)
	{
		if (version == 0) {
			throw std::runtime_error("(de)-serialization of PyMarocco::Defects not supported");
		}
		using boost::serialization::make_nvp;
		ar & make_nvp("wafer", mWafer);
	}
};

} // pymarocco

BOOST_CLASS_VERSION(pymarocco::Defects, 1)
