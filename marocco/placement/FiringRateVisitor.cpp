#include "marocco/placement/FiringRateVisitor.h"
#include "marocco/Logger.h"
#include <algorithm>

namespace marocco {
namespace placement {

FiringRateVisitor::FiringRateVisitor(pymarocco::PyMarocco const& pymarocco):
	mPyMarocco(pymarocco)
{}

typename FiringRateVisitor::return_type
FiringRateVisitor::operator() (
	cell_t<CellType::SpikeSourceArray> const& v,
	size_t neuron_id)
{
	MAROCCO_TRACE("SpikeSourceArray");

	auto const& param = v.parameters()[neuron_id];
	auto const& sts = param.spike_times;

	double rate = 0.; // pynn rate in Hz
	if ( sts.size() > 1 ) {
		static const double ms_to_s = 0.001; // PyNN spike times are in ms.

		// we allow unsorted spike trains(cf. #1733), hence we cannot use
		// front() and back()
		auto minmax = std::minmax_element(sts.begin(), sts.end());
		double t_first = *minmax.first*ms_to_s;
		double t_last = *minmax.second*ms_to_s;

		rate = (sts.size()-1)/(t_last-t_first);
	}
	else if ( sts.size() == 1 ) {
		rate = 1.;
	}

	return rate*mPyMarocco.speedup;
}

// Spike Source Poisson Transformation
typename FiringRateVisitor::return_type
FiringRateVisitor::operator() (
	cell_t<CellType::SpikeSourcePoisson> const& v,
	size_t neuron_id)
{
	MAROCCO_TRACE("SpikeSourcePoisson");

	auto const& param = v.parameters()[neuron_id];

	return param.rate*mPyMarocco.speedup; // PyNN rate is in Hz.
}

} // namespace placement
} // namespace marocco
