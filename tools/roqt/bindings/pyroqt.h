#pragma once

#include <string>

#include <boost/serialization/export.hpp>

#include "marocco/routing/Result.h"
#include "marocco/routing/results/L1Routing.h"

namespace boost {
namespace serialization {
class access;
} // namespace serialization
} // namespace boost

class PyRoQt
{
public:
	typedef marocco::routing::SynapseRoutingResult SynapseRouting;
	typedef marocco::routing::results::L1Routing L1Routing;

	PyRoQt();
	PyRoQt(L1Routing const& l1_routing, SynapseRouting const& synapse_routing);

	void load(std::string const& fname);
	void store(std::string const& fname) const;

	L1Routing l1_routing;
	SynapseRouting synapse_routing;

private:
	friend class boost::serialization::access;
	template <typename Archiver>
	void serialize(Archiver& ar, const unsigned int /* version */);
};

BOOST_CLASS_EXPORT_KEY(PyRoQt)
