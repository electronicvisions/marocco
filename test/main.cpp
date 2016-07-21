#include <iostream>
#include "test/common.h"
#include "marocco/Logger.h"

int main(int argc, char *argv[])
{
	marocco::Logger::init();

	testing::InitGoogleTest(&argc, argv);

	size_t const failure = RUN_ALL_TESTS();

	return failure;
}
