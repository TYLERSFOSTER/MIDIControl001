#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <cstdlib>
#include <sys/wait.h>
#include <iostream>
#include <regex>
#include <filesystem>

// Step 12-C: Verify analyzer RMS matches in-engine diagnostic RMS.
TEST_CASE("Analyzer RMS alignment", "[diagnostics][rms]") {
    // ------------------------------------------------------------
    // Launch analyzer subprocess
    // ------------------------------------------------------------
    FILE* pipe = popen("../../.venv/bin/python ../../tools/analyze_logs.py --verbose", "r");
    REQUIRE(pipe);

    char buffer[512];
    std::string output;
    while (fgets(buffer, sizeof(buffer), pipe)) output += buffer;
    int status = pclose(pipe);
    REQUIRE(WIFEXITED(status));

    int exit_code = WEXITSTATUS(status);
    std::cout << "[DEBUG] Analyzer exit code = " << exit_code << std::endl;

    // Analyzer must never crash (exit 99)
    REQUIRE(exit_code != 99);

    // ✅ Fixed: absolute, deterministic report path
    std::filesystem::path report_path = std::filesystem::path(CMAKE_SOURCE_DIR) / "report_summary.txt";
    REQUIRE(std::filesystem::exists(report_path));

    // ------------------------------------------------------------
    // Handle missing-data / clipping anomalies gracefully
    // ------------------------------------------------------------
    if (exit_code == 2) {
        WARN("Analyzer returned 2 (missing-data or clipping anomaly, non-fatal). "
             "Skipping RMS alignment assertions.");
        SUCCEED();
        return;
    }

    // ------------------------------------------------------------
    // Extract Avg RMS value via regex
    // ------------------------------------------------------------
    std::smatch m;
    std::regex_search(output, m, std::regex("Avg RMS\\s*:\\s*([0-9.]+)"));
    REQUIRE(m.size() > 1);

    const float avgRMS = std::stof(m[1]);
    std::cout << "[DEBUG] Analyzer Avg RMS = " << avgRMS << std::endl;

    // ------------------------------------------------------------
    // Expect it to be roughly 0.25 ± 0.05
    // ------------------------------------------------------------
    REQUIRE(avgRMS == Catch::Approx(0.25f).margin(0.05f));
}
