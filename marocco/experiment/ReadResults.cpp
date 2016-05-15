#include "marocco/experiment/ReadResults.h"

#include "hal/Coordinate/iter_all.h"

#include "marocco/Logger.h"

using namespace HMF::Coordinate;

namespace marocco {
namespace experiment {

ReadResults::ReadResults(
	pymarocco::PyMarocco const& pymarocco,
	hardware_system_t const& hardware,
	placement::results::Placement const& neuron_placement,
	BioGraph const& bio_graph,
	HMF::Coordinate::Wafer const& wafer)
	: m_pymarocco(pymarocco),
	  m_hardware(hardware),
	  m_neuron_placement(neuron_placement),
	  m_bio_graph(bio_graph),
	  m_wafer(wafer)
{
}

double ReadResults::translate(double hw_time_in_s) const
{
	return (hw_time_in_s - m_pymarocco.experiment_time_offset) * m_pymarocco.speedup;
}

void ReadResults::run(ObjectStore& objectstore) const
{
	for (auto const& population : objectstore.populations()) {
		for (auto const& item : m_neuron_placement.find(m_bio_graph[population.get()])) {
			auto const& address = item.address();
			if (address == boost::none) {
				continue;
			}
			auto const neuron_index = item.bio_neuron().neuron_index();

			HICANNGlobal hicann(address->toHICANNOnWafer(), m_wafer);
			GbitLinkOnHICANN gbit_link(address->toDNCMergerOnHICANN());
			auto const& chip = m_hardware[hicann];

			auto const& received_spikes = chip.receivedSpikes(gbit_link);
			auto const& sent_spikes = chip.sentSpikes(gbit_link);

			auto const& l1_address = address->toL1Address();
			for (auto const& spike : received_spikes) {
				if (spike.addr == l1_address) {
					population->getSpikes(neuron_index).push_back(translate(spike.time));
				}
			}

			for (auto const& spike : sent_spikes) {
				if (spike.addr == l1_address) {
					population->getSpikes(neuron_index).push_back(translate(spike.time));
				}
			}
		}
	}
}

} // namespace experiment
} // namespace marocco
