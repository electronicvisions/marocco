#pragma once

#include "sthal/Wafer.h"

#include "marocco/parameter/results/AnalogOutputs.h"

namespace marocco {
namespace experiment {

class AnalogOutputsConfigurator {
public:
	AnalogOutputsConfigurator(parameter::results::AnalogOutputs const& analog_outputs);

	void configure(sthal::Wafer& hardware) const;

private:
	parameter::results::AnalogOutputs const& m_analog_outputs;
}; // AnalogOutputsConfigurator

} // namespace experiment
} // namespace marocco
