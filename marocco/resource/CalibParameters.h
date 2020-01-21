#pragma once

#include "boost/shared_ptr.hpp"
#include "halco/hicann/v2/hicann.h"

namespace HMF {
class HICANNCollection;
class L1CrossbarCollection;
} // HMF

namespace calibtic {
namespace backend {
class Backend;
} // namespace backend
} // namespace calibtic

namespace pymarocco {
class PyMarocco;
} // pymarocco

namespace marocco {
namespace resource {

class CalibParameters
{
public:
	typedef halco::hicann::v2::HICANNGlobal hicann_type;
	typedef HMF::HICANNCollection calib_type;

	/**
	 * Constructor of the calibration loader class
	 *
	 * @param [in] hicann: used to construct the filename to load
	 * @param [in] pymarocco: used to get the calibration path, and pll_freq
	 * @param [in] calib_backend: the backend used to load calibration
	 */
	CalibParameters(
	    hicann_type const& hicann,
	    pymarocco::PyMarocco const& pymarocco,
	    boost::shared_ptr<calibtic::backend::Backend> const& calib_backend);

	/**
	 * loads calibration data from database
	 * @param [in] bool fallback_to_defailts: if loading fails decide to throw or load defaults
	 * @return boost::sharet_ptr<calib_type> returns a shared pointer to the loaded calibration.
	 **/
	boost::shared_ptr<calib_type> getCalibrationData(bool fallback_to_defaults) const;

private:
	hicann_type const& m_hicann;
	pymarocco::PyMarocco const& m_pymarocco;
	boost::shared_ptr<calibtic::backend::Backend> m_calib_backend;
};

} // namespace resource
} // namespace marocco
