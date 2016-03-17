#pragma once

#include "marocco/placement/Result.h"
#include "marocco/placement/LookupTable.h"
#include "marocco/graph.h"
#include "pymarocco/MappingStats.h"

namespace marocco {

class HardwareUsage
{
public:
	typedef hardware_system_t Hardware;
	typedef resource_manager_t Resource;
	typedef HMF::Coordinate::HICANNGlobal Index;
	typedef placement::NeuronPlacementResult::denmem_assignment_type denmem_assignment_type;

	HardwareUsage(Hardware const& hw,
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

	/// references the complete hardware system in use
	Hardware const& mHW;

	/// reference to resource manager
	Resource const& mResource;

	denmem_assignment_type const& mPlacement;

	/// (shared) pointer to the forward+reverse mapping information
	std::shared_ptr<placement::LookupTable> mLookupTable;
};

} // namespace marocco
