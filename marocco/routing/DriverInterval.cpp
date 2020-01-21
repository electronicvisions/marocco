#include "marocco/routing/DriverInterval.h"

#include <ostream>

using namespace halco::hicann::v2;

namespace marocco {
namespace routing {

DriverInterval::DriverInterval(VLineOnHICANN const& vline, size_t drivers, size_t syns) :
	line(vline),
	driver(drivers),
	synapses(syns),
	intervals()
{
	if (!driver) {
		throw std::runtime_error("empty driver interval");
	}
}

std::ostream& operator<<(std::ostream& os, DriverInterval const& i)
{
	os << "DriverInterval(" <<  i.line << " driver: "
		<< i.driver << " inserts: " << i.intervals.size() << " synapses: " << i.synapses;
	for(auto const& val : i.intervals) {
		os << val.first << " ";
	}
	return os << ")";
}

} // namespace routing
} // namespace marocco
