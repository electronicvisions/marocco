#include "marocco/HardwareUsage.h"
#include "marocco/Result.h"
#include "marocco/Logger.h"
#include "hal/Coordinate/iter_all.h"

#include "HMF/SynapseDecoderDisablingSynapse.h"

using namespace HMF::Coordinate;

namespace marocco {

HardwareUsage::HardwareUsage(sthal::Wafer const& hardware, Resource const& r, BaseResult const& pl)
	: mHW(hardware),
	  mResource(r),
	  mNeuronPlacement(result_cast<placement::Result>(pl).neuron_placement)
{}

double HardwareUsage::overallNeuronUsage() const
{
	size_t denmems = 0;
	size_t hicanns = 0;
	for (auto const& hicann : mResource.allocated()) {
		size_t denmems_on_hicann = 0;
		for (auto const& item : mNeuronPlacement.find(hicann)) {
			denmems_on_hicann += item.logical_neuron().size();
		}
		if (denmems_on_hicann > 0) {
			denmems += denmems_on_hicann;
			++hicanns;
		}
	}
	if (hicanns > 0) {
		return double(denmems) / double(hicanns * NeuronOnHICANN::enum_type::size);
	}
	return 0.;
}

double HardwareUsage::overallSynapseUsage() const
{
	size_t synapses = 0;
	size_t hicanns = 0;
	for (auto const& hicann : mResource.allocated()) {
		// Check if any neurons have been placed to this HICANN.
		if (!mNeuronPlacement.find(hicann).empty()) {
			synapses += numSynapsesUsed(hicann);
			++hicanns;
		}
	}
	if (hicanns > 0) {
		return double(synapses) / double(hicanns * SynapseOnHICANN::enum_type::size);
	}
	return 0.;
}

size_t HardwareUsage::numSynapsesUsed(Index const& hicann) const
{
	auto& chip = getChip(hicann);

	size_t cnt=0;
	for (auto const& syn : iter_all<SynapseOnHICANN>())
	{
		auto const& proxy = chip.synapses[syn];
		if (proxy.decoder != HMF::HICANN::SynapseDecoderDisablingSynapse) {
			cnt++;
		}
	}
	return cnt;
}

size_t HardwareUsage::numSynapseDriverUsed(Index const& hicann) const
{
	auto& chip = getChip(hicann);

	size_t cnt=0;
	for (auto const& drv : iter_all<SynapseDriverOnHICANN>())
	{
		HMF::HICANN::SynapseDriver const& driver = chip.synapses[drv];
		if (driver.is_enabled()) {
			cnt++;
		}
	}
	return cnt;
}

size_t HardwareUsage::numDenmemsUsed(Index const& hicann) const
{
	auto& chip = getChip(hicann);

	size_t cnt=0;
	for (auto const& nrn : iter_all<NeuronOnHICANN>())
	{
		auto const& neuron = chip.neurons[nrn];
		if (neuron.activate_firing() || neuron.enable_fire_input()) {
			cnt++;
		}
	}
	return cnt;
}

size_t HardwareUsage::numVLinesUsed(Index const& hicann) const
{
	auto& chip = getChip(hicann);

	size_t cnt=0;
	for (auto const& vline : iter_all<VLineOnHICANN>())
	{
		auto const& repeater = chip.repeater[vline.toVRepeaterOnHICANN()];
		if (repeater.getMode() != HMF::HICANN::Repeater::IDLE) {
			cnt++;
		}
	}
	return cnt;
}

size_t HardwareUsage::numHLinesUsed(Index const& hicann) const
{
	size_t cnt=0;

	auto& chip = getChip(hicann);
	for (auto const& hline : iter_all<HLineOnHICANN>())
	{
		auto const& repeater = chip.repeater[hline.toHRepeaterOnHICANN()];
		if (repeater.getMode() != HMF::HICANN::Repeater::IDLE) {
			cnt++;
		}
	}
	return cnt;
}

void HardwareUsage::fill(pymarocco::MappingStats& stats) const
{
	stats.setNeuronUsage(overallNeuronUsage());
	stats.setSynapseUsage(overallSynapseUsage());
}

sthal::HICANN const& HardwareUsage::getChip(Index const& hicann) const
{
	return mHW[hicann];
}

} // namespace marocco
