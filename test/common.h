#pragma once

#include <gtest/gtest.h>
#include <stdexcept>
#include <cstdlib>
#include <ctime>

// ester MPI config
#include "mpi/config.h"

#ifdef GTEST_CASE
#define GTEST_CASE
#endif

#include "marocco/test.h"

inline
int provided_thread_level(int init = MPI_THREAD_SINGLE)
{
	static const int _level = init;
	return _level;
}

inline
bool threaded_mpi()
{
	return provided_thread_level() >= MPI_THREAD_SERIALIZED;
}

#define BREAK_POINT do { __asm__ ("int $0x3\n"); } while (false)

#define REPEAT(N, ...) \
	std::srand(std::time(nullptr)); \
	for (size_t __ii=0; __ii<N; ++__ii) { __VA_ARGS__ }
