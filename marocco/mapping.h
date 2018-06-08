#pragma once
#include <mpi.h>
#include <boost/shared_ptr.hpp>
#include <boost/mpi/intercommunicator.hpp>

#include "marocco/Mapper.h"

#include "control/Control.h"
#include "experiment/Result.h"

#include "mappingresult.h"
#include "pymarocco/PyMarocco.h"

namespace marocco {
namespace mapping {

std::set<HMF::Coordinate::Wafer> wafers_used_in(boost::shared_ptr<ObjectStore> store);

MappingResult run(boost::shared_ptr<ObjectStore> store,
                  Mapper::comm_type const& comm = MPI::COMM_WORLD);

} // namespace mapping
} // namespace marocco
