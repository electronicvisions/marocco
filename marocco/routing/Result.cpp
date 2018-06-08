#include "marocco/routing/Result.h"

#include <fstream>
#include <ostream>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include "marocco/Logger.h"

namespace marocco {
namespace routing {

std::ostream& SynapseRowRoutingResult::operator<< (std::ostream& o) const
{
	o << typestring(*this) << "(";
	for (auto& it : this->_mapping)
	{
		o << "Chip: [" << it.first;
		o << "] Result(size: " << it.second.size();
		for(auto& iit : it.second)
		{
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
