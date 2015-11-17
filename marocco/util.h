#pragma once
#include <stdexcept>

namespace marocco {

class Error : public std::runtime_error {};
class ResourceInUseError : public Error {};

} // namespace marocco
