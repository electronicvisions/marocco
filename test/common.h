#pragma once

#include <gtest/gtest.h>
#include <stdexcept>
#include <cstdlib>
#include <ctime>

#ifdef GTEST_CASE
#define GTEST_CASE
#endif

#include "marocco/test.h"

#define BREAK_POINT do { __asm__ ("int $0x3\n"); } while (false)

#define REPEAT(N, ...) \
	std::srand(std::time(nullptr)); \
	for (size_t __ii=0; __ii<N; ++__ii) { __VA_ARGS__ }
