#include "marocco/experiment/SpikeTimesConfigurator.h"

#include "halco/common/iter_all.h"

#include "marocco/Logger.h"

using namespace halco::hicann::v2;
using namespace halco::common;

namespace marocco {
namespace experiment {

SpikeTimesConfigurator::SpikeTimesConfigurator(
	placement::results::Placement const& placement,
	parameter::results::SpikeTimes const& spike_times,
	parameters::Experiment const& experiment_parameters)
	: m_placement(placement),
	  m_spike_times(spike_times),
	  m_experiment_parameters(experiment_parameters)
{
}

void SpikeTimesConfigurator::configure(sthal::Wafer& hardware, HICANNOnWafer const& hicann) const
{
	static double const ms_to_s = 1e-3;
	double const speedup = m_experiment_parameters.speedup();
	double const offset_in_s = m_experiment_parameters.offset_in_s();
	double const bio_duration_in_ms = m_experiment_parameters.bio_duration_in_s() / ms_to_s;

	for (auto const dnc_merger : iter_all<DNCMergerOnHICANN>()) {
		std::vector<sthal::Spike> transformed_spikes;
		for (auto const& item : m_placement.find(DNCMergerOnWafer(dnc_merger, hicann))) {
			if (!item.logical_neuron().is_external()) {
				continue;
			}

			auto const& address = item.address();
			assert(address != boost::none);
			auto const& l1_address = address->toL1Address();

			auto const& spikes = m_spike_times.get(item.bio_neuron());
			transformed_spikes.reserve(transformed_spikes.size() + spikes.size());
			for (auto const& bio_time_in_ms : spikes) {
				if (bio_time_in_ms > bio_duration_in_ms) {
					continue;
				}
				transformed_spikes.emplace_back(
					l1_address, offset_in_s + bio_time_in_ms * ms_to_s / speedup);
			}
		}

		if (!transformed_spikes.empty()) {
			MAROCCO_DEBUG(
			    "Sending " << transformed_spikes.size() << " spikes to " << hicann << " via "
			    << dnc_merger);
			hardware[hicann].sendSpikes(GbitLinkOnHICANN(dnc_merger), transformed_spikes);
		}
	}
}

} // namespace experiment
} // namespace marocco
