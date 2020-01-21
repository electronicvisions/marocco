#pragma once

#include "sthal/Wafer.h"
#include "halco/hicann/v2/hicann.h"

#include "marocco/parameter/results/SpikeTimes.h"
#include "marocco/experiment/parameters/Experiment.h"
#include "marocco/placement/results/Placement.h"

namespace marocco {
namespace experiment {

class SpikeTimesConfigurator {
public:
	SpikeTimesConfigurator(
		placement::results::Placement const& placement,
		parameter::results::SpikeTimes const& spike_times,
		parameters::Experiment const& experiment_parameters);

	void configure(sthal::Wafer& hardware, halco::hicann::v2::HICANNOnWafer const& hicann) const;

private:
	placement::results::Placement const& m_placement;
	parameter::results::SpikeTimes const& m_spike_times;
	parameters::Experiment const& m_experiment_parameters;
}; // SpikeTimesConfigurator

} // namespace experiment
} // namespace marocco
