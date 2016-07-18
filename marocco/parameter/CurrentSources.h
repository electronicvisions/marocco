#pragma once

#include <unordered_map>

#include "euter/current.h"
#include "sthal/Wafer.h"

#include "marocco/BioGraph.h"
#include "marocco/placement/results/Placement.h"
#include "pymarocco/PyMarocco.h"

namespace marocco {
namespace parameter {

class CurrentSources {
public:
	typedef std::unordered_map<BioNeuron, ConstCurrentSourcePtr> current_sources_type;

	CurrentSources(
		sthal::Wafer& hardware, placement::results::Placement const& neuron_placement);

	void run(current_sources_type const& current_sources);

private:
	sthal::Wafer& m_hardware;
	placement::results::Placement const& m_neuron_placement;
}; // CurrentSources

} // namespace parameter
} // namespace marocco
