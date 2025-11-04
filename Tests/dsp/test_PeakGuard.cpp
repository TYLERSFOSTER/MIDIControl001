#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
using Catch::Approx;

#include "dsp/PeakGuard.h"

// ============================================================
// PeakGuard Unit Test — realistic envelope behavior
// ============================================================
//
// 1. Verify limiting at 2.0f input (hard clip region)
// 2. Verify that after reset (or cooldown), moderate inputs pass unchanged
// ============================================================

TEST_CASE("PeakGuard soft limiting behavior", "[peakguard]") {
    PeakGuard pg;
    pg.prepare(48000.0, 10.0f);

    // --- Step 1: limit a hot sample ---
    float sample = 2.0f;
    float out = pg.process(sample);
    REQUIRE(out <= 1.0f);
    REQUIRE(out > 0.0f);
    REQUIRE(pg.getEngagementRatio() > 0.0f);

    // --- Step 2: let envelope decay or reset before testing clean path ---
    pg.reset();  // isolates the next section (alternative to cooldown loop)

    // --- Step 3: verify low-level samples pass through unchanged ---
    for (int i = 0; i < 100; ++i) {
        float in = 0.5f;
        float out2 = pg.process(in);
        REQUIRE(out2 == Approx(in).margin(0.05f));
    }

    // --- Step 4: ensure limiter remains disengaged ---
    REQUIRE(pg.getEngagementRatio() == Approx(0.0f).margin(0.01f));
}
