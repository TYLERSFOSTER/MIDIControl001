#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <cstdlib>
#include <sys/wait.h>
#include <iostream>
#include <regex>

// Step 12-C: Verify analyzer RMS matches in-engine diagnostic RMS.
TEST_CASE("Analyzer RMS alignment", "[diagnostics][rms]") {
    // Run analyzer and capture output.
    FILE* pipe = popen("../../.venv/bin/python ../../tools/analyze_logs.py --verbose", "r");
    REQUIRE(pipe);

    char buffer[512];
    std::string output;
    while (fgets(buffer, sizeof(buffer), pipe)) output += buffer;
    int status = pclose(pipe);
    REQUIRE(WIFEXITED(status));

    // Extract RMS value via regex.
    std::smatch m;
    std::regex_search(output, m, std::regex("Avg RMS\\s*:\\s*([0-9.]+)"));
    REQUIRE(m.size() > 1);
    const float avgRMS = std::stof(m[1]);

    std::cout << "[DEBUG] Analyzer Avg RMS = " << avgRMS << std::endl;

    // Expect it to be roughly 0.25 ± 0.05.
    REQUIRE(avgRMS == Catch::Approx(0.25f).margin(0.05f));
}
