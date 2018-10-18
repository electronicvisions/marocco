#pragma once

#include <string>
#include "boost/shared_ptr.hpp"

namespace redman {
namespace backend {
class Backend;
}
}

namespace marocco {
namespace resource {

class BackendLoaderRedman
{
public:
	template <typename Backend>
	static boost::shared_ptr<redman::backend::Backend> load_redman_backend(
	    std::string& defect_path);
};

} // namespace resource
} // namespace marocco
