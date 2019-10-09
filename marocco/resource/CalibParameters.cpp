#include "marocco/resource/CalibParameters.h"

#include "hal/Coordinate/HICANN.h"

#include "calibtic/HMF/HICANNCollection.h"
#include "calibtic/backend/Backend.h"

#include "marocco/Logger.h"
#include "pymarocco/PyMarocco.h"

using namespace HMF::Coordinate;

namespace marocco {
namespace resource {

CalibParameters::CalibParameters(
    hicann_type const& hicann,
    pymarocco::PyMarocco const& pymarocco,
    boost::shared_ptr<calibtic::backend::Backend> const& calib_backend)
    : m_hicann(hicann), m_pymarocco(pymarocco), m_calib_backend(calib_backend)
{}

boost::shared_ptr<CalibParameters::calib_type> CalibParameters::getCalibrationData(
    bool fallback_to_defaults) const
{
	auto calib = boost::make_shared<calib_type>();
	if (!m_calib_backend) {
		MAROCCO_INFO("no backend: setting defaults");
		calib->setDefaults();
	} else {
		calibtic::MetaData md;

		const size_t hicann_id = m_hicann.toHICANNOnWafer().id().value();
		std::stringstream calib_file;
		calib_file << "w" << size_t(m_hicann.toWafer()) << "-h";
		calib_file << hicann_id;
		const std::string calib_file_string = calib_file.str();

		MAROCCO_TRACE(
		    "loading calibration for " << calib_file_string << " from " << m_pymarocco.calib_path);
		try {
			m_calib_backend->load(calib_file_string, md, *calib);
		} catch (std::runtime_error const& err) {
			if (!fallback_to_defaults) {
				throw;
			}
			MAROCCO_WARN(err.what());
			MAROCCO_WARN("Will use default calibtration");
			calib->setDefaults();
		}
	}

	if (calib->getPLLFrequency() != m_pymarocco.pll_freq) {
		MAROCCO_WARN(
		    "PLL stored in HICANNCollection " << int(calib->getPLLFrequency() / 1e6)
		                                      << " MHz != " << int(m_pymarocco.pll_freq / 1e6)
		                                      << " MHz set here.");
	}

	return calib;
}

} // namespace resource
} // namespace marocco
