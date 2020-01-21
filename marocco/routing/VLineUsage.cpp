#include "marocco/routing/VLineUsage.h"

using namespace halco::hicann::v2;

namespace {

size_t synapse_switch_period(VLineOnHICANN const& line)
{
	size_t period =
	    (line.value() / SynapseSwitchOnHICANN::period_length) % SynapseSwitchOnHICANN::periods;
	if (line.isRight()) {
		// Right side has access to a different set of synapse drivers.
		period += SynapseSwitchOnHICANN::periods;
	}
	return period;
}

} // namespace

namespace marocco {
namespace routing {

VLineUsage::VLineUsage()
	: m_usage()
{
}

void VLineUsage::increment(
    halco::hicann::v2::HICANNOnWafer const& hicann, halco::hicann::v2::VLineOnHICANN const& vline)
{
	m_usage[hicann][synapse_switch_period(vline)]++;
}

size_t VLineUsage::get(
    halco::hicann::v2::HICANNOnWafer const& hicann, halco::hicann::v2::VLineOnHICANN const& vline) const
{
	return m_usage[hicann][synapse_switch_period(vline)];
}

} // namespace routing
} // namespace marocco
