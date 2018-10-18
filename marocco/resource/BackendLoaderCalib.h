#pragma once

#include <string>
#include "boost/shared_ptr.hpp"

namespace calibtic {
namespace backend {
class Backend;
}
}


namespace marocco {

namespace resource {

class BackendLoaderCalib
{
public:
	template <typename Backend>
	static boost::shared_ptr<calibtic::backend::Backend> load_calibtic_backend(
	    std::string calib_path);
};

} // namespace resource

} // namespace marocco
