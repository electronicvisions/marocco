#include "marocco/routing/Result.h"

#include <fstream>
#include <ostream>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include "marocco/Logger.h"

namespace marocco {
namespace routing {

std::ostream& SynapseRoutingResult::operator<<(std::ostream& o) const
{
	o << typestring(*this) << "(";
	for (auto& it : this->_mapping)
	{
		o << "Chip: [" << it.first;
		std::vector<DriverResult> const& driver_result = it.second.driver_result;
		o << "] DriverResult(size: " << driver_result.size();
		for (auto& iit : driver_result) {
			// FIXME: Coordinate::SynapseDriverOnHICANN printer doesn't work somehow
			//o << " SynapseDriver(" << iit.first.side() << " " << iit.first.line() << ")" << std::endl;
			////o << iit.first;
			//o << " Source: " << iit.second;
		}
	}
	o << "))";
	return o;
}

} // namespace routing
} // namespace marocco
