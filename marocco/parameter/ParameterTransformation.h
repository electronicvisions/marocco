#pragma once

#include <type_traits>
#include <stdexcept>

#include "marocco/graph.h"
#include "marocco/Algorithm.h"

namespace marocco {
namespace parameter {

class ParameterTransformation :
	public Algorithm
{
public:
	virtual ~ParameterTransformation() {};
	template<typename ... Args>
	ParameterTransformation(Args&& ... args) :
		Algorithm(std::forward<Args>(args)...) {}

	// placement start interface
	virtual std::unique_ptr<result_type> run(
		result_type const& placement,
		result_type const& routing) = 0;
};

} // namespace parameter
} // namespace marocco
