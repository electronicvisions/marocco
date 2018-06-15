#include "marocco/parameter/SpikeInputVisitor.h"

#include <iterator>

#include "marocco/Logger.h"

using namespace HMF::Coordinate;

namespace marocco {
namespace parameter {

void SpikeInputVisitor::operator()(
	cell_t<CellType::SpikeSourceArray> const& v,
	size_t neuron_id,
	size_t const /* seed */,
	double const /* experiment_duration */,
	spikes_type& spikes) const
{
	auto const& param = v.parameters()[neuron_id];

	spikes.reserve(param.spike_times.size());
	// TODO: Should we check for time < experiment_duration here?
	std::copy(param.spike_times.begin(), param.spike_times.end(), std::back_inserter(spikes));

	MAROCCO_TRACE("added " << spikes.size() << " spikes from SpikeSourceArray");
}

// Spike Source Poisson Transformation
void SpikeInputVisitor::operator()(
	cell_t<CellType::SpikeSourcePoisson> const& v,
	size_t neuron_id,
	size_t const seed,
	double const experiment_duration,
	spikes_type& spikes) const
{
	auto const& param = v.parameters()[neuron_id];

	// Rate (in Hz) has to be adjusted because spike times are stored in ms.
	static double const s_to_ms = 1e3;
	std::exponential_distribution<double> dist(param.rate / s_to_ms);

	std::mt19937 rng(seed);
	double time = param.start;
	double const stop = time + std::min(param.duration, experiment_duration);

	spikes.reserve(2 * dist.lambda() * experiment_duration);
	while (true) {
		time += dist(rng);
		if (time >= stop) {
			break;
		}
		spikes.push_back(time);
	}

	MAROCCO_TRACE("added " << spikes.size() << " spikes from " << param);
}

SpikeInputVisitor::spikes_type extract_input_spikes(
	Population const& pop,
	size_t const neuron_id,
	size_t const seed,
	double const experiment_duration)
{
	SpikeInputVisitor::spikes_type spikes;
	SpikeInputVisitor visitor{};

	visitCellParameterVector(
		pop.parameters(), visitor, neuron_id, seed, experiment_duration, spikes);

	return spikes;
}

} // namespace parameter
} // namespace marocco
