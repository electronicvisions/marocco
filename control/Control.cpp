#include "control/Control.h"
#include "marocco/Logger.h"
#include <memory>

#include "sthal/MagicHardwareDatabase.h"
#include "sthal/HICANNConfigurator.h"
#include "sthal/HICANNv4Configurator.h"
#include "sthal/DontProgramFloatingGatesHICANNConfigurator.h"
#include "sthal/ExperimentRunner.h"
#include "sthal/ESSHardwareDatabase.h"
#include "sthal/ESSRunner.h"
#include <fstream>
#include <boost/filesystem.hpp>

using namespace marocco;
using namespace HMF::Coordinate;

namespace control {

Control::Control(hardware_type& hw,
				 resource_manager_t const& rmgr,
				 pymarocco::PyMarocco& pym) :
	mHW(hw),
	mMgr(rmgr),
	mPyMarocco(pym)
{}

void Control::run(const double duration_in_s /*seconds*/)
{
	//for (auto const& hicann : mMgr.getUsed())
	//{
		//logChip(hicann);
	//}

	auto const wafers = mMgr.wafers();
	assert(wafers.size() == 1);
	auto const wafer = wafers.front();

	std::unique_ptr<sthal::ExperimentRunner> runner;
	std::unique_ptr<sthal::HardwareDatabase> db;

	pickRunner(runner, db, duration_in_s);

	dumpWafer(wafer);

	auto config = pickHICANNConfigurator();

	if (mPyMarocco.backend == Backend::None) {
		return;
	}

	mHW.connect(*db);
	mHW[wafer].commonFPGASettings()->setPLL(mPyMarocco.pll_freq);
	mHW.configure(*config);

	if (mPyMarocco.membrane.empty()) {

		runExperiment(runner);

	} else {

		double const record_time_stretch_factor = 3.;
		double const record_time = duration_in_s * record_time_stretch_factor;

		runExperimentAndRecordMembrane(runner, mPyMarocco.hicann_enum,
		                               mPyMarocco.analog_enum, record_time);
	}
}

void Control::logChip(HICANNGlobal const& h) const
{
	auto const& chip = mHW[h];

	MAROCCO_INFO(chip.layer1);
	MAROCCO_INFO(chip.crossbar_switches);
	MAROCCO_INFO(chip.synapse_switches);
	MAROCCO_INFO(chip.repeater);
	MAROCCO_INFO(chip.synapses);
}


// creates a a directory with beginning "/tmp/ess_", which does not yet exists, and returns path as string
std::string create_unique_tmp_directory() {
	using namespace boost::filesystem;
	path tmp_dir;
	bool created_by_me = false;
	do {
		tmp_dir  = temp_directory_path()/unique_path("ess_%%%%-%%%%-%%%%-%%%%");
		created_by_me = create_directory(tmp_dir); // returns true if a new directory was created, otherwise false.
	}
	while ( !created_by_me );
	return tmp_dir.native();
}

// deletes directory in destructor
class DirectoryDeleter {
public:
	DirectoryDeleter(std::string path): mPath(path){}
	virtual ~DirectoryDeleter() {
		using namespace boost::filesystem;
		remove_all(path(mPath)); // deletes if exists
	}
private:
	std::string mPath;
};

static std::vector<DirectoryDeleter> directory_deleters;

void Control::pickRunner(std::unique_ptr<sthal::ExperimentRunner>& runner,
						 std::unique_ptr<sthal::HardwareDatabase>& db,
                       	 double dur_in_s) const {

	if (mPyMarocco.backend == Backend::None) {
		MAROCCO_INFO("Backend: None");
		return;
	} else if (mPyMarocco.backend == Backend::ESS) {
#ifdef HAVE_ESS
		MAROCCO_INFO("Backend: ESS " << dur_in_s << "s");
		std::string ess_dir = mPyMarocco.ess_temp_directory;
		// if directory was not specified by the user:
		// create unique temp directory and register it to be deleted at the end of
		// the program
		if ( ess_dir.empty() ) {
			ess_dir = create_unique_tmp_directory();
			directory_deleters.emplace_back( ess_dir );
		}
		db.reset(new sthal::ESSHardwareDatabase(ess_dir));
		runner.reset(new sthal::ESSRunner(dur_in_s));
#else
		throw std::runtime_error("ESS not available (compile with ESS)");
#endif
	} else if (mPyMarocco.backend == Backend::Hardware) {
		MAROCCO_INFO("Backend: Hardware " << dur_in_s << "s");
		db.reset(new sthal::MagicHardwareDatabase());
		runner.reset(new sthal::ExperimentRunner(dur_in_s));
	} else {
		throw std::runtime_error("unknown backend");
	}

}

sthal::HICANNConfigurator* Control::pickHICANNConfigurator() const {

	switch(mPyMarocco.hicann_configurator) {

	case HICANNCfg::HICANNConfigurator:
		return new sthal::HICANNConfigurator();
		break;

	case HICANNCfg::HICANNv4Configurator:
		return new sthal::HICANNv4Configurator();
		break;

	case HICANNCfg::DontProgramFloatingGatesHICANNConfigurator:
		return new sthal::DontProgramFloatingGatesHICANNConfigurator();
		break;

	default:
		return new sthal::HICANNConfigurator();
	}

}

void Control::dumpWafer(HMF::Coordinate::Wafer const& coord) const {

	if (!mPyMarocco.wafer_cfg.empty()) {
		auto& wafer = mHW[coord];
		wafer.dump(mPyMarocco.wafer_cfg.c_str(), true);
	}

}

void Control::runExperiment(std::unique_ptr<sthal::ExperimentRunner>& runner) const {

	MAROCCO_INFO("run experiment");
	mHW.start(*runner);

}

void Control::runExperimentAndRecordMembrane(std::unique_ptr<sthal::ExperimentRunner>& runner,
                                             size_t hicann_global_enum,
                                             size_t analog_enum,
                                             double record_in_s) const {

	HICANNGlobal const hicann{Enum(hicann_global_enum)};
	auto& h = mHW[hicann];
	auto recorder = h.analogRecorder(AnalogOnHICANN(Enum(analog_enum)));
	MAROCCO_INFO("recording membrane for: " << record_in_s << " s");
	recorder.activateTrigger(record_in_s);

	runExperiment(runner);

	auto const trace = recorder.trace();
	auto const times = recorder.getTimestamps();

	static sthal::AnalogRecorder::time_type const seconds_to_ms = 1000.;
	static sthal::AnalogRecorder::voltage_type const V_to_mV = 1000.;

	std::ofstream file(mPyMarocco.membrane.c_str());
	for (size_t ii=0; ii<trace.size(); ++ii) {

		auto const hw_time_in_s = times[ii];
		auto const hw_trace_in_V = trace[ii];

		if (mPyMarocco.membrane_translate_to_bio) {

			// using explicit types to make clear that we will write that type to disk
			sthal::AnalogRecorder::time_type const t_in_bio_ms =
			    (hw_time_in_s - mPyMarocco.experiment_time_offset) * mPyMarocco.speedup *
			    seconds_to_ms;
			sthal::AnalogRecorder::voltage_type const membrane_in_bio_mV =
			    (hw_trace_in_V * V_to_mV - mPyMarocco.param_trafo.shift_v) /
			    mPyMarocco.param_trafo.alpha_v;

			file << t_in_bio_ms << " " << membrane_in_bio_mV << std::endl;

		} else {
			file << hw_time_in_s << " " << hw_trace_in_V << std::endl;
		}
	}
}

} // config
