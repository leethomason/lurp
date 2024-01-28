#pragma once

int RunTests();	// returns failures
int TestReturnCode();

void RecordTest(bool pass);
void IncTestRun();

#define RUN_TEST(test) IncTestRun(); fmt::print("Test: {}\n", #test); test

// Like assert(), but works in release mode too
// always runs the test, so that the behavior doesn't change.
#define TEST(x)                                                 \
	if (!(x)) {	                                                \
		RecordTest(false);                                       \
		printf("ERROR: line %d in %s\n", __LINE__, __FILE__);   \
        assert(false);                                          \
	}															\
	else {														\
		RecordTest(true);										\
	}

#define TEST_FP(x, y)                                           \
	if (fabs(x - y) > 0.0001) {	                                \
		RecordTest(false);                                       \
		printf("ERROR: line %d in %s\n", __LINE__, __FILE__);   \
        assert(false);                                          \
	}                                                           \
	else {														\
		RecordTest(true);										\
	}

