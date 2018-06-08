#include <iostream>
#include "test/common.h"
#include "marocco/Logger.h"

int main(int argc, char *argv[])
{
	marocco::Logger::init();

	// try to initialize MPI in thread mode. Required thread level is at least
	// MPI_THREAD_SERIALIZED, because gtests runs tests in a non-main thread.
	int const required = MPI_THREAD_SERIALIZED;
	int const provided = MPI::Init_thread(argc, argv, required);

	// publish provided thread level.
	provided_thread_level(provided);

	if (provided < required) {
		std::cerr << "Your MPI installation is compiled without required"
		   " thread support. MPI tests run by gtest requires at least"
		   " `MPI_THREAD_SERIALIZED`. Some tests will be deactivated."
		   << std::endl;
	}

	testing::InitGoogleTest(&argc, argv);

	size_t const failure = RUN_ALL_TESTS();

	MPI::Finalize();

	return failure;
}
