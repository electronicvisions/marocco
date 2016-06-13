#include "marocco/parameter/SpikeInputVisitor.h"

#include "hal/Coordinate/iter_all.h"

#include "marocco/Logger.h"

using namespace HMF::Coordinate;

namespace marocco {
namespace parameter {

void transform_input_spikes(
	Population const& pop,
	HMF::HICANN::L1Address const& l1,
	size_t neuron_id,
	chip_type<hardware_system_t>::type& chip,
	SpikeInputVisitor& visitor)
{
	// configure analog neuron parameters
	visitCellParameterVector(
		pop.parameters(),
		visitor,
		l1,
		neuron_id,
		chip);
}

SpikeInputVisitor::SpikeInputVisitor(
			pymarocco::PyMarocco const& pymarocco,
			SpikeList& spikes, int seed, double exp_dur) :
	mPyMarocco(pymarocco),
	mSpikes(spikes),
	mRNG(seed),
	mExperimentDuration(exp_dur)
{}

typename SpikeInputVisitor::return_type
SpikeInputVisitor::operator() (
	cell_t<CellType::SpikeSourceArray> const& v,
	HMF::HICANN::L1Address const& l1,
	size_t neuron_id,
	chip_t& /*unused*/)
{
	const double speedup = mPyMarocco.speedup;
	const double time_offset = mPyMarocco.experiment_time_offset;

	auto const& param = v.parameters()[neuron_id];

	mSpikes.reserve(param.spike_times.size());
	for (double const time : param.spike_times) {
		if (time < mExperimentDuration) {
			mSpikes.emplace_back(l1, time_offset + time / speedup / 1000.0);
		}
	}

	MAROCCO_DEBUG("added " << mSpikes.size() << " spikes from SpikeSourceArray");
}

// Spike Source Poisson Transformation
typename SpikeInputVisitor::return_type
SpikeInputVisitor::operator() (
	cell_t<CellType::SpikeSourcePoisson> const& v,
	HMF::HICANN::L1Address const& l1,
	size_t neuron_id,
	chip_t& /*unused*/)
{
	const double speedup = mPyMarocco.speedup;
	const double time_offset = mPyMarocco.experiment_time_offset;

	auto const& param = v.parameters()[neuron_id];

	// Rate has to be divided by 1000 because time is given in ms.
	std::exponential_distribution<double> dist(param.rate / 1000.0);

	double time = param.start;
	// spike train stops either at start + duration or at end of pynn experiment
	double const stop = std::min(param.start + param.duration, mExperimentDuration);

	while (true) {
		time += dist(mRNG);
		if (time >= stop) {
			break;
		}
		mSpikes.emplace_back(l1, time_offset + time / speedup / 1000.0);
	}

	MAROCCO_DEBUG("added " << mSpikes.size() << " spikes from " << param);
}

} // namespace parameter
} // namespace marocco
