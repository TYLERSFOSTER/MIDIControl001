// [Lifecycle: Active]
// [Subsystem: DSP/Debug]
// [Purpose: Validate JSON baseline serialization and integrity]

#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <iostream>

TEST_CASE("Voice baseline JSON key dump", "[voice][baseline][debug]") {
    namespace fs = std::filesystem;
    auto jsonPath = fs::path(__FILE__).parent_path()
                    / ".." / "baseline" / "voice_output_reference.json";

    std::ifstream f(jsonPath);
    REQUIRE(f.good());

    nlohmann::json j;
    f >> j;

    std::cout << "\n[Baseline JSON keys @ "
              << jsonPath.filename().string() << "]\n";

    for (auto it = j.begin(); it != j.end(); ++it)
        std::cout << it.key() << "\n";
}
