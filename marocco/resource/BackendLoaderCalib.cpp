#include "BackendLoaderCalib.h"

#include "boost/shared_ptr.hpp"
#include "calibtic/backend/Library.h"
#include "calibtic/backends/binary/BinaryBackend.h"

namespace calibtic {
namespace backend {
class XMLBackend;
class BinaryBackend;
} // backend
} // calibtic

namespace marocco {
namespace resource {


template <>
boost::shared_ptr<calibtic::backend::Backend>
BackendLoaderCalib::load_calibtic_backend<calibtic::backend::XMLBackend>(std::string calib_path)
{
	boost::shared_ptr<calibtic::backend::Library> lib;

	lib = calibtic::backend::loadLibrary("libcalibtic_xml.so");

	auto backend = calibtic::backend::loadBackend(lib);

	if (!backend) {
		throw std::runtime_error("unable to load calib backend");
	}

	if (std::getenv("MAROCCO_CALIB_PATH") != nullptr) {
		if (!calib_path.empty())
			// we break hard, if the user specified via both ways...
			throw std::runtime_error(
			    "colliding settings: environment variable and pymarocco.calib_path both set");
		calib_path = std::string(std::getenv("MAROCCO_CALIB_PATH"));
	}

	backend->config("path", calib_path); // search in calib_path for calibration xml files
	backend->init();
	return backend;
}

template <>
boost::shared_ptr<calibtic::backend::Backend>
BackendLoaderCalib::load_calibtic_backend<calibtic::backend::BinaryBackend>(std::string calib_path)
{
	boost::shared_ptr<calibtic::backend::Library> lib;

	lib = calibtic::backend::loadLibrary("libcalibtic_binary.so");

	auto backend = calibtic::backend::loadBackend(lib);

	if (!backend) {
		throw std::runtime_error("unable to load calib backend");
	}

	if (std::getenv("MAROCCO_CALIB_PATH") != nullptr) {
		if (!calib_path.empty())
			// we break hard, if the user specified via both ways...
			throw std::runtime_error(
			    "colliding settings: environment variable and pymarocco.calib_path both set");
		calib_path = std::string(std::getenv("MAROCCO_CALIB_PATH"));
	}

	backend->config("path", calib_path); // search in calib_path for calibration xml files
	backend->init();
	return backend;
}

} // namespace resource
} // namespace marocco
