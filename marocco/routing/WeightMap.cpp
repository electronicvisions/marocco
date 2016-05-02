#include "marocco/routing/WeightMap.h"
#include "hal/Coordinate/HMFGeometry.h"
#include <limits>

using namespace HMF::Coordinate;

namespace marocco {
namespace routing {

WeightMap::WeightMap(
    routing_graph const& rg, placement::WaferL1AddressAssignment const& address_assignment)
	: g(rg), mAddressAssignment(address_assignment)
{
	for (auto& val : horizontal) { val = 0; }
	for (auto& val : vertical) { val = 0; }
}

WeightMap::WeightMap(pymarocco::Routing const& param, routing_graph const& rg,
					 placement::WaferL1AddressAssignment const& address_assignment) :
	g(rg),
	w_vertical(param.weight_Vertical),
	w_horizontal(param.weight_Horizontal),
	w_SPL1(param.weight_SPL1),
	w_straight_horizontal(param.weight_StraightHorizontal),
	w_straight_vertical(param.weight_StraightVertical),
	w_congestion_factor(param.weight_CongestionFactor),
	mAddressAssignment(address_assignment)
{
	for (auto& val : horizontal) { val = 0; }
	for (auto& val : vertical) { val = 0; }
}

WeightMap::reference WeightMap::operator[] (key_type const& k) const
{
	auto const& target = g[boost::target(k, g)];

	size_t const id = target.hicann().toHICANNOnWafer().id();

	if (target.getDirection() == L1Bus::Horizontal) {
		HRepeaterOnHICANN const hrepeater(Enum(target.getBusId()));
		if (hrepeater.isSending()) {
			// return highes possible weigt if SPL1 is needed for later routing
			HICANNGlobal const& hicann = target.hicann();
			DNCMergerOnHICANN const& merger =
			    hrepeater.toSendingRepeaterOnHICANN().toDNCMergerOnHICANN();
			auto it = mAddressAssignment.find(hicann);
			if (it != mAddressAssignment.end() && !mAddressAssignment.at(hicann).is_unused(merger)) {
				// max() leads to overflows and therefore cycles...
				// return std::numeric_limits<reference>::max();
				return 100000;
			}
			return w_congestion_factor * horizontal[id] + w_horizontal;
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
