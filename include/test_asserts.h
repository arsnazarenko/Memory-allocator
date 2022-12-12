#ifndef TEST_ASSERTS_H
#define TEST_ASSERTS_H

#include <assert.h>

/**
 * Create a basic test function. Call it with RUN_TEST()
 */
#define TEST(Name, Subname) static void test_##Name##_##Subname()

/**
 * Run a previously created test function
 */
#define RUN_TEST(Name, Subname) test_##Name##_##Subname()

/**
 * Assert that an expression is true
 */
#define ASSERT_TRUE(Exp) do { assert(Exp); } while (0)

/**
 * Assert that two expressions are equal. They need to be of the same type
 */
#define ASSERT_EQ(Lhs, Rhs) do { assert(Lhs == Rhs); } while (0)

/**
 * todo: macros for print
 * _____________________
 * TEST 1 "NAME" STARTED 
 * in Out stream
 */ 
#define TEST_PASSED(Name, Subname, Out) do { fputs("")  } while (0)


/**
 * todo: macros for print
 * TEST 1 "NAME" PASSED
 * in Out stream
 */ 
#define TEST_PASSED(Name, Subname, Out) do { fputs("")  } while (0)

#endif /* TEST_ASSERTS_H */
