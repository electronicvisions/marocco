#pragma once
#include <boost/shared_ptr.hpp>

#include "hal/Coordinate/Wafer.h"
#include "euter/objectstore.h"
#include "mappingresult.h"

namespace marocco {
namespace mapping {

std::set<HMF::Coordinate::Wafer> wafers_used_in(boost::shared_ptr<ObjectStore> store);

MappingResult run(boost::shared_ptr<ObjectStore> store);

} // namespace mapping
} // namespace marocco
