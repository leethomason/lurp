#pragma once

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <fmt/core.h>

// Like assert, but doesn't get removed in release builds
#define CHECK(x)                                                \
	if (!(x)) {	                                                \
		printf("ERROR: CHECK failed. Line %d in %s\n", __LINE__, __FILE__);   \
		assert(false);                                          \
	}

inline void FatalError(const std::string & msg)
{
	fmt::print("[ERROR] {}\n", msg);
	assert(false);
	exit(2);
}

#define FATAL_INTERNAL_ERROR() \
	FatalError(fmt::format("Internal error at '{}' in '{}'. Please report this to the developers.\n", __LINE__, __FILE__));
