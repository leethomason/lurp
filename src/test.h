#pragma once

#include <plog/Log.h>

int RunTests();	// returns failures
int TestReturnCode();

void RecordTest(bool pass);
void IncTestRun();
void LogTestResults();

#define RUN_TEST(test) IncTestRun(); PLOG(plog::info) << "Test: " << #test; test

// Like assert(), but works in release mode too
// always runs the test, so that the behavior doesn't change.
#define TEST(x)                                                 \
	if (!(x)) {	                                                \
		RecordTest(false);                                      \
		PLOG(plog::error) << "Test failure line " << __LINE__ << " in " << __FILE__; \
        assert(false);                                          \
	}															\
	else {														\
		RecordTest(true);										\
	}

#define TEST_FP(x, y)                                           \
	if (fabs(x - y) > 0.0001) {	                                \
		RecordTest(false);                                       \
		PLOG(plog::error) << "Test failure line " << __LINE__ << " in " << __FILE__; \
        assert(false);                                          \
	}                                                           \
	else {														\
		RecordTest(true);										\
	}

