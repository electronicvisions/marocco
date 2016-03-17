#include "marocco/HardwareUsage.h"
#include "marocco/Result.h"
#include "marocco/Logger.h"
#include "hal/Coordinate/iter_all.h"

#include "HMF/SynapseDecoderDisablingSynapse.h"

using namespace HMF::Coordinate;

namespace marocco {

HardwareUsage::HardwareUsage(Hardware const& hw, Resource const& r, BaseResult const& pl)
	: mHW(hw),
	  mResource(r),
	  mPlacement(result_cast<placement::Result>(pl).neuron_placement.denmem_assignment()),
	  mLookupTable(result_cast<placement::Result>(pl).reverse_mapping)
{}

double HardwareUsage::overallNeuronUsage() const
{
	size_t cnt=0, num_hicanns=0;
	for (auto const& hicann: mResource.allocated())
	{
		if (mPlacement.find(hicann) != mPlacement.end()) {
			size_t c=0;
			for (auto const& nb : iter_all<NeuronBlockOnHICANN>())
			{
				auto const& onb = mPlacement.at(hicann).at(nb);
				for (std::shared_ptr<placement::NeuronPlacementRequest> const& pl : onb) {
					c += pl->size();
				}
			}
			cnt += c;
			num_hicanns++;
		}
	}
	return double(cnt) / double(num_hicanns*NeuronOnHICANN::enum_type::end);
}

double HardwareUsage::overallSynapseUsage() const
{
	size_t cnt=0, num_hicanns=0;
	for (auto const& hicann: mResource.allocated())
	{
		if (mPlacement.find(hicann) != mPlacement.end()) {
			cnt += numSynapsesUsed(hicann);
			num_hicanns++;
		}
	}
	return double(cnt) / double(num_hicanns*SynapseOnHICANN::enum_type::end);
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
	stats.setLookupTable(mLookupTable);
}

sthal::HICANN const& HardwareUsage::getChip(Index const& hicann) const
{
	return mHW[hicann];
}

} // namespace marocco
