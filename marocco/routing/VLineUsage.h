#pragma once

#include <array>

#include "halco/hicann/v2/hicann.h"
#include "halco/hicann/v2/l1.h"
#include "halco/common/typed_array.h"

namespace marocco {
namespace routing {

/**
 * @brief Tracks usage of VLineOnHICANNs as insertion point.
 * Groups of 16 VLineOnHICANNs share the same 14 SynapseDriverOnHICANNs, thus no
 * one-to-one assignment is possible and we may have to refrain from using a bus, if it is
 * already overused.
 */
class VLineUsage {
	typedef halco::common::typed_array<
	    std::array<size_t, 2 * halco::hicann::v2::SynapseSwitchOnHICANN::periods>,
	    halco::hicann::v2::HICANNOnWafer>
	    array_type;

public:
	VLineUsage();

	static constexpr size_t max()
	{
		static_assert(
		    halco::hicann::v2::SynapseSwitchOnHICANN::per_column / 2 == 14,
		    "unexpected number of reachable synapse drivers");
		return halco::hicann::v2::SynapseSwitchOnHICANN::per_column / 2;
	}

	void increment(
		halco::hicann::v2::HICANNOnWafer const& hicann,
	    halco::hicann::v2::VLineOnHICANN const& vline);

	size_t get(
		halco::hicann::v2::HICANNOnWafer const& hicann,
		halco::hicann::v2::VLineOnHICANN const& vline) const;

private:
	array_type m_usage;
}; // VLineUsage

} // namespace routing
} // namespace marocco
