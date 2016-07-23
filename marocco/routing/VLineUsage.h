#pragma once

#include <array>

#include "hal/Coordinate/HICANN.h"
#include "hal/Coordinate/L1.h"
#include "hal/Coordinate/typed_array.h"

namespace marocco {
namespace routing {

/**
 * @brief Tracks usage of VLineOnHICANNs as insertion point.
 * Groups of 16 VLineOnHICANNs share the same 14 SynapseDriverOnHICANNs, thus no
 * one-to-one assignment is possible and we may have to refrain from using a bus, if it is
 * already overused.
 */
class VLineUsage {
	typedef HMF::Coordinate::typed_array<
	    std::array<size_t, 2 * HMF::Coordinate::SynapseSwitchOnHICANN::periods>,
	    HMF::Coordinate::HICANNOnWafer>
	    array_type;

public:
	VLineUsage();

	static constexpr size_t max()
	{
		static_assert(
		    HMF::Coordinate::SynapseSwitchOnHICANN::per_column / 2 == 14,
		    "unexpected number of reachable synapse drivers");
		return HMF::Coordinate::SynapseSwitchOnHICANN::per_column / 2;
	}

	void increment(
		HMF::Coordinate::HICANNOnWafer const& hicann,
	    HMF::Coordinate::VLineOnHICANN const& vline);

	size_t get(
		HMF::Coordinate::HICANNOnWafer const& hicann,
		HMF::Coordinate::VLineOnHICANN const& vline) const;

private:
	array_type m_usage;
}; // VLineUsage

} // namespace routing
} // namespace marocco
