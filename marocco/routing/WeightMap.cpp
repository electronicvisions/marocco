#include "marocco/routing/WeightMap.h"
#include "hal/Coordinate/HMFGeometry.h"
#include <limits>

using namespace HMF::Coordinate;

namespace marocco {
namespace routing {

WeightMap::WeightMap(routing_graph const& rg, placement::OutputMappingResult const& outbm) :
	g(rg), mOutbMapping(outbm)
{
	for (auto& val : horizontal) { val = 0; }
	for (auto& val : vertical) { val = 0; }
}

WeightMap::WeightMap(pymarocco::Routing const& param, routing_graph const& rg,
					 placement::OutputMappingResult const& outbm) :
	g(rg),
	w_vertical(param.weight_Vertical),
	w_horizontal(param.weight_Horizontal),
	w_SPL1(param.weight_SPL1),
	w_straight_horizontal(param.weight_StraightHorizontal),
	w_straight_vertical(param.weight_StraightVertical),
	w_congestion_factor(param.weight_CongestionFactor),
	mOutbMapping(outbm)
{
	for (auto& val : horizontal) { val = 0; }
	for (auto& val : vertical) { val = 0; }
}

WeightMap::reference WeightMap::operator[] (key_type const& k) const
{
	auto const& source = g[boost::source(k, g)];
	auto const& target = g[boost::target(k, g)];

	size_t const id = target.hicann().toHICANNOnWafer().id();

	if (target.getDirection() == L1Bus::Horizontal) {
		HRepeaterOnHICANN const hrepeater(Enum(target.getBusId()));
		if (hrepeater.isSending()) {
			// return highes possible weigt if SPL1 is needed for later routing
			HICANNGlobal const& hicann = target.hicann();
			OutputBufferOnHICANN const& outb = hrepeater.toSendingRepeaterOnHICANN().toOutputBufferOnHICANN();
			auto it = mOutbMapping.find(hicann);
			if (it!=mOutbMapping.end() && !mOutbMapping.at(hicann).empty(outb)) {
				// max() leads to overflows and therefore cycles...
				//return std::numeric_limits<reference>::max();
				return 100000;
			}
			return w_congestion_factor*horizontal[id] + w_horizontal;
		}
		//else if(source.getDirection() == L1Bus::Horizontal) {
			//// favour straight connectivity
			//return w_congestion_factor*horizontal[id] + w_straight_horizontal;
		//}
		else {
			return w_congestion_factor*horizontal[id] + w_horizontal;
		}
	} else { // target vertical
		//if(source.getDirection() == L1Bus::Vertical) {
			//// favour straight connectivity
			//return w_congestion_factor*vertical[id] + w_straight_vertical;
		//}
		//else {
			return w_congestion_factor*vertical[id] + w_vertical;
		//}
	}
}

} // namespace routing
} // namespace marocco
