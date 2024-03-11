#pragma once

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <plog/Log.h>

// Like assert, but doesn't get removed in release builds
#define CHECK(x)                                                \
	if (!(x)) {	                                                \
		PLOG(plog::error) << "CHECK runtime assertion failed";  \
		assert(false);                                          \
	}

inline void FatalError(const std::string & msg)
{
	PLOG(plog::error) << "FATAL: " << msg;
	assert(false);							
	exit(2);
}

#define FATAL_INTERNAL_ERROR() \
	FatalError(fmt::format("Internal error at '{}' in '{}'. Please report this to the developers.\n", __LINE__, __FILE__));
