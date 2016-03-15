#include "marocco/placement/NeuronBlockMapping.h"

#include <algorithm>

namespace marocco {
namespace placement {

NeuronBlockMapping::NeuronBlockMapping() :
	mMapping()
{}

OnNeuronBlock&
NeuronBlockMapping::at(index const& idx)
{
	return mMapping.at(idx);
}

OnNeuronBlock const&
NeuronBlockMapping::at(index const& idx) const
{
	return mMapping.at(idx);
}

OnNeuronBlock&
NeuronBlockMapping::operator[] (index const& idx) noexcept
{
	return mMapping[idx];
}

OnNeuronBlock const&
NeuronBlockMapping::operator[] (index const& idx) const noexcept
{
	return mMapping[idx];
}

bool NeuronBlockMapping::any() const
{
	return std::any_of(mMapping.begin(), mMapping.end(),
		[](Mapping::value_type const& l) { return !l.empty(); });
}

size_t NeuronBlockMapping::neurons(index const& idx) const
{
	size_t nrns = 0;
	for (std::shared_ptr<NeuronPlacementRequest> const& pl : mMapping[idx]) {
		nrns += pl->population_slice().size();
	}
	return nrns;
}

size_t NeuronBlockMapping::available() const
{
	size_t cnt = 0;
	for (auto const& v : mMapping)
		cnt += v.available();
	return  cnt;
}

} // namespace placement
} // namespace marocco
