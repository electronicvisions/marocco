#pragma once

#include <functional>
#include <unordered_map>

#include "euter/objectstore.h"
#include "sthal/Wafer.h"

#include "marocco/BioGraph.h"
#include "marocco/config.h"
#include "marocco/coordinates/LogicalNeuron.h"
#include "marocco/experiment/parameters/Experiment.h"
#include "marocco/results/Marocco.h"
#include "pymarocco/PyMarocco.h"

namespace marocco {
namespace experiment {

class Experiment
{
public:
	typedef placement::results::Placement::item_type placement_item_type;

	Experiment(
		sthal::Wafer& hardware,
		results::Marocco const& results,
	    BioGraph const& bio_graph,
		parameters::Experiment const& parameters,
		pymarocco::PyMarocco const& pymarocco,
		sthal::ExperimentRunner& experiment_runner,
		sthal::HardwareDatabase& hardware_database,
		sthal::HICANNConfigurator& configurator);

	void run();

	/**
	 * @brief Populate objectstore with spike times and membrane voltage traces.
	 * @pre The experiment must have been executed via #run().
	 */
	void extract_results(ObjectStore& objectstore) const;

private:
	bool extract_spikes(PopulationPtr population, placement_item_type const& item) const;

	sthal::Wafer& m_hardware;
	results::Marocco const& m_results;
	BioGraph const& m_bio_graph;
	parameters::Experiment const& m_parameters;
	pymarocco::PyMarocco const& m_pymarocco;

	sthal::ExperimentRunner& m_experiment_runner;
	sthal::HardwareDatabase& m_hardware_database;
	sthal::HICANNConfigurator& m_configurator;
	std::unordered_map<LogicalNeuron, sthal::AnalogRecorder> m_analog_recorders;
};

} // namespace experiment
} // namespace marocco
