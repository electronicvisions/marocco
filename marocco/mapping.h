#pragma once
#include <boost/shared_ptr.hpp>

#include "marocco/Mapper.h"

#include "mappingresult.h"
#include "pymarocco/PyMarocco.h"

namespace marocco {
namespace mapping {

std::set<HMF::Coordinate::Wafer> wafers_used_in(boost::shared_ptr<ObjectStore> store);

MappingResult run(boost::shared_ptr<ObjectStore> store);

} // namespace mapping
} // namespace marocco
