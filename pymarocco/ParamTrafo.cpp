#include "pymarocco/ParamTrafo.h"

namespace pymarocco {

ParamTrafo::ParamTrafo():
	use_big_capacitors(true),
	alpha_v(10.),
	shift_v(1.2)
	{}

void ParamTrafo::set_trafo_with_bio_to_hw_ratio(
    double const max_experiment_voltage,
    double const min_experiment_voltage,
    double const max_hw_voltage,
    double const min_hw_voltage)
{
	double const V_to_mV = 1000.;
	alpha_v = V_to_mV * (max_hw_voltage - min_hw_voltage) / (max_experiment_voltage - min_experiment_voltage);
	shift_v = (max_experiment_voltage * min_hw_voltage - min_experiment_voltage * max_hw_voltage) / (max_experiment_voltage - min_experiment_voltage);
}

double ParamTrafo::calculate_hw_value(double const bio_value)
{
	double const mV_to_V = 1. / 1000.;
	return (alpha_v * bio_value * mV_to_V + shift_v);
}
} // pymarocco
