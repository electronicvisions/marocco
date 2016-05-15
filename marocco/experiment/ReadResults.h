#pragma once

#include "hal/Coordinate/Wafer.h"

#include "marocco/config.h"
#include "marocco/BioGraph.h"
#include "marocco/placement/results/Placement.h"
#include "pymarocco/PyMarocco.h"

namespace marocco {
namespace experiment {

class ReadResults
{
public:
	ReadResults(
	    pymarocco::PyMarocco const& pymarocco,
	    hardware_system_t const& hardware,
	    placement::results::Placement const& neuron_placement,
	    BioGraph const& bio_graph,
	    HMF::Coordinate::Wafer const& wafer);

	void run(ObjectStore& objectstore) const;

private:
	/// translate hardware spike times back to the biological time domain.
	/// @param hw_time_in_s time of hardware spike in seconds
	/// @returns bio spike time in seconds
	double translate(double hw_time_in_s) const;

	pymarocco::PyMarocco const& m_pymarocco;
	hardware_system_t const& m_hardware;
	placement::results::Placement const& m_neuron_placement;
	BioGraph const& m_bio_graph;
	HMF::Coordinate::Wafer const& m_wafer;
}; // ReadResults

} // namespace experiment
} // namespace marocco
