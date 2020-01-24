#pragma once
#include <boost/shared_ptr.hpp>

#include "halco/hicann/v2/wafer.h"
#include "euter/objectstore.h"

namespace marocco {
namespace mapping {

std::set<halco::hicann::v2::Wafer> wafers_used_in(euter::ObjectStore& store);

void run(euter::ObjectStore& store);

} // namespace mapping
} // namespace marocco
