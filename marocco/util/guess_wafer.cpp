#include "marocco/util/guess_wafer.h"

namespace marocco {

halco::hicann::v2::Wafer guess_wafer(resource::HICANNManager const& mgr)
{
	assert(mgr.wafers().size() == 1);
	return mgr.wafers().front();
}

} // namespace marocco
