#include <boost/program_options.hpp>
#include <boost/make_shared.hpp>
#include <boost/filesystem.hpp>
#include <boost/regex.hpp>
#include <boost/version.hpp>
#include <boost/mpi/intercommunicator.hpp>
#include <iostream>
#include <chrono>
#include <memory>
#include <signal.h>

#include "marocco/mapping.h"
#include "marocco/Logger.h"

// global variables

// file name of input script, if specified by cmd line
boost::filesystem::path fname;

// if no hardware is present, only carry out mapping:
bool hw_present;

using namespace std;
using namespace marocco;
using namespace pymarocco;
namespace mpi = boost::mpi;


namespace std {
ostream& operator<< (ostream& os, vector<string> const& vec)
{
	for (auto const& v : vec)
		os << v << ", ";
	return os << endl;
}
} // std

void terminate(int signal)
{
	info() << "marocco's signal handler called with SIG: " << signal;
	if (signal == SIGSEGV || signal == SIGFPE) {
		if (MPI::Is_initialized() && fname.empty()) {
			//parent.Send(&signal, 1, MPI::INT, 0, msg_finished_mapping);
			MPI::Finalize();
		}
		exit(-1);
	}
}

void finalize(chrono::time_point<chrono::system_clock>& start_time)
{
	using namespace std::chrono;
	auto duration = duration_cast<milliseconds>(system_clock::now()-start_time);
	info() << "finalizing mpi (" << duration.count() << " ms)";
	MPI::Finalize();
}

// spins until master is released by debugger
// attach with e.g.: gdb and set attached true
void wait_for_debugger(bool wait)
{
	if (wait) {
		int rank = MPI::COMM_WORLD.Get_rank();
		bool attached = false;

		if (rank == MASTER_PROCESS) {
			while (!attached) {}
		}

		MPI::COMM_WORLD.Bcast(&attached, 1, MPI::BOOL, MASTER_PROCESS);
	}
}

void install_signal_handler(bool set)
{
	if (set) { // install signal handler
		signal (SIGSEGV, terminate);
		signal (SIGFPE,  terminate);
		signal (SIGTERM, terminate);
	}
}

void init_logger(vector<string> const& levels)
{
	boost::regex const e("([^\\:]*?):([^\\:\\.]*?)");

	vector<pair<string, log4cxx::LevelPtr>> arg;
	for (auto const& lvl : levels)
	{
		boost::cmatch cm;
		boost::regex_match(lvl.c_str(), cm, e);

		if (cm.size() != 3)
			throw std::runtime_error("invalid log level cmd line argument");
		arg.push_back(make_pair(cm[1], log4cxx::Level::toLevel(cm[2])));
	}
	marocco::Logger::init(arg);
}

int main(int argc, char** argv)
{
	// bootstrapping MPI
	MPI::Init_thread(argc, argv, MPI_THREAD_SERIALIZED);
	MPI::COMM_WORLD.Set_errhandler(MPI::ERRORS_THROW_EXCEPTIONS);
	size_t const rank = MPI::COMM_WORLD.Get_rank();

	namespace po = boost::program_options;
	po::options_description desc("Allowed options");
	desc.add_options()
		("help", "produce this help message")
		("debug",      po::bool_switch()->notifier(&wait_for_debugger),
			 "spin until debugger attaches")
		("input-file", po::value<boost::filesystem::path>(&fname)->default_value(""),
			 "file to read serialized PyNN script from")
		("sig",        po::bool_switch()->notifier(&install_signal_handler),
			 "install signal handlers to cushion most mapping crashes")
		("hw",         po::value<bool>(&hw_present)->default_value(false),
			 "actual hardware is present")
		("loglevel",   po::value<vector<string>>()->default_value({"marocco::all"})->notifier(&init_logger)->multitoken(),
			 "set individual log levels ala <logger:level>");

	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);

	if (vm.count("help")) {
		cerr << desc << endl;
		return EXIT_FAILURE;
	}

	auto start_time = chrono::system_clock::now();

	// get parent and set MPI error handler
	mpi::intercommunicator parent(MPI::COMM_WORLD.Get_parent(), mpi::comm_take_ownership);
	if (parent) {
		MPI_Comm_set_errhandler(parent, MPI::ERRORS_THROW_EXCEPTIONS);
	} else if (fname.empty()) {
		MPI::COMM_WORLD.Abort(-1);
		throw runtime_error("missing pyNN network (no MPI nor file)");
	}

	// receive pyNN script
	auto objectstore = boost::make_shared<ObjectStore>();
	if (rank == MASTER_PROCESS) {
		if (fname.empty()) {
			parent.recv(0, msg_data_package, *objectstore);
		} else {
			ifstream ifs(fname.string().c_str());
			boost::archive::binary_iarchive oa(ifs);
			oa >> *objectstore;
		}
	}

	auto const wafers = mapping::wafers_used_in(objectstore);

	size_t const np = min(size_t(MPI::COMM_WORLD.Get_size()), wafers.size());
	MPI::Intracomm comm = MPI::COMM_WORLD.Split(rank<np, rank);

	// M A P P I N G
	// start actual mapping process
	//   needs three inputs:
	//      * pyNN script
	//      * corresponding mapping support structure (implicitly holds
	//        reference to hardware)
	//   returns pynn/mapping-graph

	auto result = mapping::run(objectstore, comm);

	// TODO: implement something meaningful
	// signal ester server results and state
	if (rank == MASTER_PROCESS && fname.empty()) {
		parent.send(0, msg_finished_mapping, result);
	} else {
		// write to file?
	}

	// clean up
	finalize(start_time);
	return EXIT_SUCCESS;
}
