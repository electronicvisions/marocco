#include "pyroqt.h"

#include <fstream>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

PyRoQt::PyRoQt()
{}

PyRoQt::PyRoQt(CrossbarRouting const& cb,
			   SynapseRowRouting const& sr,
			   Graph const& g) :
	mCrossbarRouting(cb),
	mSynapseRowRouting(sr),
	mRoutingGraph(g)
{}

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

PyRoQt::CrossbarRouting const&
PyRoQt::crossbar() const
{
	return mCrossbarRouting;
}

PyRoQt::SynapseRowRouting const&
PyRoQt::synapserow() const
{
	return mSynapseRowRouting;
}

PyRoQt::Graph const& PyRoQt::graph() const
{
	return mRoutingGraph;
}

marocco::routing::L1Bus const&
PyRoQt::getL1Bus(Graph::vertex_descriptor const& v) const
{
	return mRoutingGraph[v];
}
