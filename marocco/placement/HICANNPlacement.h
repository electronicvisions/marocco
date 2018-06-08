#pragma once

#include <map>
#include <list>
#include "marocco/placement/Placement.h"
#include "marocco/placement/SpiralHICANNOrdering.h"
#include "marocco/placement/DescendingPopulationOrdering.h"
#include "marocco/util.h"
#include "marocco/test.h"

namespace pymarocco {
class Placement;
}

namespace marocco {
namespace placement {

class SpiralTerminalOrdering
{
public:
	typedef HMF::Coordinate::NeuronBlockGlobal Terminal;

	bool operator() (Terminal const& a, Terminal const& b) const
	{
		static const SpiralHICANNOrdering _s;
		if (a.toHICANNGlobal() == b.toHICANNGlobal()) {
			return a.toNeuronBlockOnHICANN() < b.toNeuronBlockOnHICANN();
		}
		return _s(a.toHICANNGlobal(), b.toHICANNGlobal());
	}
};


class HICANNPlacement
{
private:
	typedef graph_t::vertex_descriptor vertex;
	//typedef std::set<assignment::PopulationSlice, DescendingPopulationOrdering> set_pop;
	typedef std::list<NeuronPlacement> set_pop;

	typedef HMF::Coordinate::NeuronBlockGlobal Terminal;
	typedef std::set<Terminal, SpiralTerminalOrdering> TerminalList;

public:
	HICANNPlacement(
		pymarocco::Placement const& pl,
		graph_t const& nn,
		hardware_system_t const& hw,
		resource_manager_t& mgr,
		comm_t const& comm);

	std::unique_ptr<NeuronPlacementResult> run();

private:
	struct NoNeuronBlockFound {};

	/** runs the actual placement of yet unassigned populations to slices of hardware blocks
	 */
	void place2(
		std::map<size_t, TerminalList>& tlist,
		set_pop& pops,
		NeuronPlacementResult& res);

	void place2_(
		std::list<Terminal>& terminals,
		set_pop& pops,
		NeuronPlacementResult& res);

	void insert(
		HMF::Coordinate::HICANNGlobal const& hicann,
		HMF::Coordinate::NeuronBlockOnHICANN const& nb,
		NeuronPlacement const& assign,
		assignment::Hardware::offset_type const& offset,
		NeuronBlockMapping& hw,
		PlacementMap& bio);

	/** Returns the number of still available hardware neurons on a neuron block
	 */
	size_t available(
		NeuronPlacementResult const& res, //!< intermediate placement result
		HMF::Coordinate::HICANNGlobal const& hicann, //!< HICANN coordinate
		HMF::Coordinate::NeuronBlockOnHICANN const& nb //!< neuron on block coordinate
		) const;

	pymarocco::Placement const& mPyPlacement;
	graph_t const&            mGraph;
	hardware_system_t const&  mHW;
	resource_manager_t&       mMgr;
	comm_t                    mComm;

	FRIEND_TEST(HICANNPlacement, Basic);
};

} // namespace placement
} // namespace marocco
