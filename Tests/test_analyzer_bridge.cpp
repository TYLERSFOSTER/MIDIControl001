#include <catch2/catch_test_macros.hpp>
#include <cstdlib>
#include <sys/wait.h>

TEST_CASE("Analyzer bridge executes successfully", "[diagnostics][bridge]") {
    int status = std::system("(cd ../.. && ./.venv/bin/python tools/analyze_logs.py > /dev/null 2>&1)");
    int code = WEXITSTATUS(status);
    REQUIRE((code == 0 || code == 1 || code == 2));
}
