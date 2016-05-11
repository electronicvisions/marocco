#include "pyroqt.h"

#include <fstream>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/serialization/nvp.hpp>

PyRoQt::PyRoQt()
{
}

PyRoQt::PyRoQt(L1Routing const& l1_routing, SynapseRouting const& synapse_routing)
	: l1_routing(l1_routing), synapse_routing(synapse_routing)
{
}

void PyRoQt::load(std::string const& fname)
{
	std::ifstream fs(fname);
	boost::archive::binary_iarchive ia(fs);
	ia >> *this;
}

void PyRoQt::store(std::string const& fname) const
{
	std::ofstream fs(fname);
	boost::archive::binary_oarchive oa(fs);
	oa << *this;
}

template <typename Archiver>
void PyRoQt::serialize(Archiver& ar, const unsigned int /* version */)
{
	using namespace boost::serialization;
	// clang-format off
	ar & make_nvp("l1_routing", l1_routing)
	   & make_nvp("synapse_routing", synapse_routing);
	// clang-format on
}

BOOST_CLASS_EXPORT_IMPLEMENT(PyRoQt)

#include "boost/serialization/serialization_helper.tcc"
EXPLICIT_INSTANTIATE_BOOST_SERIALIZE(PyRoQt)
