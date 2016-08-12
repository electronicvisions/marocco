#pragma once

#include "sthal/Wafer.h"

#include "marocco/placement/Result.h"
#include "marocco/graph.h"
#include "pymarocco/MappingStats.h"

namespace marocco {

class HardwareUsage
{
public:
	typedef resource_manager_t Resource;
	typedef HMF::Coordinate::HICANNGlobal Index;

	HardwareUsage(sthal::Wafer const& hardware,
				  Resource const& r,
				  BaseResult const& pl);

	/// only counts neuron on HICANNs actually in use (meaning: neurons
	/// are placed to it), not all available.
	double overallNeuronUsage() const;

	/// only counts synapse usage on HICANNs actually in use (meaning: neurons
	/// are placed to it), not all available.
	double overallSynapseUsage() const;

	size_t numSynapsesUsed(Index const& hicann) const;
	size_t numSynapseDriverUsed(Index const& hicann) const;
	size_t numDenmemsUsed(Index const& hicann) const;
	size_t numVLinesUsed(Index const& hicann) const;
	size_t numHLinesUsed(Index const& hicann) const;

	void fill(pymarocco::MappingStats& stats) const;

private:
	sthal::HICANN const& getChip(Index const& hicann) const;

	sthal::Wafer const& mHW;

	/// reference to resource manager
	Resource const& mResource;

	placement::results::Placement const& mNeuronPlacement;
};

} // namespace marocco
