#include "../test_macros.h"
#include <plan9cog/plan9cog.h>
#include <stdlib.h>

void test_api_initialization() {
    TEST_CASE("API Initialization");
    int result = plan9cog_init();
    ASSERT(result == 0);
}

void test_api_shutdown() {
    TEST_CASE("API Shutdown");
    int result = plan9cog_shutdown();
    ASSERT(result == 0);
}

void test_api_execute_atomese() {
    TEST_CASE("API Execute Atomese");
    const char* atomese = "(Evaluation (PredicateNode \"test\") (ListLink (ConceptNode \"arg1\")))";
    char* output = plan9cog_execute_atomese(atomese);
    ASSERT(output != NULL);
    // This is a mock response, a real implementation would require a running cogserver
    ASSERT_STR_EQ(output, "(stv 1.0 1.0)");
    free(output);
}

int main() {
    test_api_initialization();
    test_api_execute_atomese();
    test_api_shutdown();
    return TEST_SUMMARY();
}
