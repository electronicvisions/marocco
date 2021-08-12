#include "BackendLoaderRedman.h"

#include "boost/make_shared.hpp"
#include "boost/shared_ptr.hpp"
#include "redman/backend/Library.h"
#include "redman/backend/MockBackend.h"

namespace redman {
namespace backend {
class Without;
class Backend;
class MockBackend;
class XMLBackend;
}
}


namespace marocco {

namespace resource {

template <>
boost::shared_ptr<redman::backend::Backend>
BackendLoaderRedman::load_redman_backend<redman::backend::XMLBackend>(std::string& defects_path)
{
	auto const lib = redman::backend::loadLibrary("libredman_xml.so");
	auto const backend = redman::backend::loadBackend(lib);

	if (!backend) {
		throw std::runtime_error("unable to load xml backend");
	}

	if (std::getenv("MAROCCO_DEFECTS_PATH") != nullptr) {
		if (!defects_path.empty()) {
			throw std::runtime_error("only one of pymarocco.defects.path and MAROCCO_DEFECTS_PATH "
			                         "should be set");
		}
		defects_path = std::string(std::getenv("MAROCCO_DEFECTS_PATH"));
	}

	backend->config("path", defects_path);
	backend->init();
	return backend;
}


template <>
boost::shared_ptr<redman::backend::Backend>
BackendLoaderRedman::load_redman_backend<redman::backend::Without>(std::string& defects_path)
{
	(void) defects_path; // supress unused variable warning

	return boost::make_shared<redman::backend::MockBackend>();
}


} // namespace resource
} // namespace marocco
