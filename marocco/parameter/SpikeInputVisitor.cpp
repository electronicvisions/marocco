#include "marocco/parameter/SpikeInputVisitor.h"
#include "marocco/Logger.h"

#include "hal/Coordinate/iter_all.h"

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
	MAROCCO_TRACE("SpikeSourceArray");
	const double speedup = mPyMarocco.speedup;
	const double time_offset = mPyMarocco.experiment_time_offset;

	auto const& param = v.parameters()[neuron_id];

	for (double const time : param.spike_times)
		// use euter's spike_times (cell parameter), transform and attach to
		// this' SpikeList -> used by HICANNTransformator
		mSpikes.emplace_back(l1, time_offset+time/speedup/1000.);
}

// Spike Source Poisson Transformation
typename SpikeInputVisitor::return_type
SpikeInputVisitor::operator() (
	cell_t<CellType::SpikeSourcePoisson> const& v,
	HMF::HICANN::L1Address const& l1,
	size_t neuron_id,
	chip_t& /*unused*/)
{
	MAROCCO_TRACE("SpikeSourcePoisson");
	const double speedup = mPyMarocco.speedup;
	const double time_offset = mPyMarocco.experiment_time_offset;

	auto const& param = v.parameters()[neuron_id];

	double const duration = param.duration;
	double const start = param.start;
	double const rate = param.rate;

   std::mt19937 gen(mRNG());
   std::uniform_real_distribution<> dis(0, 1);
	static double const dt = 0.1;

	double time = start;
	// spike train stops either at start+duration or at end of pynn experiment
	double const stop = std::min(start + duration, mExperimentDuration);

	while ( time < stop )
	{
		// time is given in ms => factor of 1000.
		if (rate * dt / 1000. >= dis(gen)) {
		//if (rate * dt >= mRNG()) {
			auto const t = time_offset+time/speedup/1000.;
			mSpikes.emplace_back(l1, t);
		}
		time+=dt;
	}
}

} // namespace parameter
} // namespace marocco
