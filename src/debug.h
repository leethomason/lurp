#pragma once

#include <assert.h>
#include <math.h>
#include <stdio.h>

// Like assert, but doesn't get removed in release builds
#define CHECK(x)                                                \
	if (!(x)) {	                                                \
		printf("ERROR: CHECK failed. Line %d in %s\n", __LINE__, __FILE__);   \
		assert(false);                                          \
	}
