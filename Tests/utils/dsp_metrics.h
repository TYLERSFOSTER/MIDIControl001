#pragma once
#include <vector>
#include <cmath>
#include <numeric>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

// ============================================================
// Utility: hashBuffer / computeRMS / computePeak / writeJson
// ============================================================

inline std::string hashBuffer(const std::vector<float>& buf)
{
    std::ostringstream ss;
    uint64_t h = 1469598103934665603ull;  // FNV-1a 64-bit offset basis
    for (float f : buf)
    {
        auto u = static_cast<uint64_t>(std::fabs(f) * 1e6);
        h ^= u;
        h *= 1099511628211ull;             // FNV prime
    }
    ss << std::hex << std::setw(16) << std::setfill('0') << h;
    return ss.str();
}

inline float computeRMS(const std::vector<float>& buf)
{
    double sumSq = 0.0;
    for (float f : buf)
        sumSq += f * f;
    return static_cast<float>(std::sqrt(sumSq / buf.size()));
}

inline float computePeak(const std::vector<float>& buf)
{
    float maxVal = 0.0f;
    for (float f : buf)
        if (std::fabs(f) > maxVal)
            maxVal = std::fabs(f);
    return maxVal;
}

inline void writeJson(const std::string& filename,
                      const std::string& hash,
                      float rms,
                      float peak)
{
    namespace fs = std::filesystem;
    fs::path outPath(filename);

    // ensure parent directory exists
    if (auto parent = outPath.parent_path();
        !parent.empty() && !fs::exists(parent))
        fs::create_directories(parent);

    nlohmann::json j;
    j["hash"] = hash;
    j["rms"]  = rms;
    j["peak"] = peak;

    std::ofstream outFile(filename);
    if (!outFile.is_open())
    {
        std::cerr << "[writeJson] Failed to open: "
                  << fs::absolute(outPath) << "\n";
        return;
    }

    outFile << std::setw(2) << j << std::endl;
    outFile.close();

    std::cout << "[writeJson] Wrote "
              << fs::absolute(outPath)
              << " (" << fs::file_size(outPath) << " bytes)\n"
              << std::flush;
}

inline bool compareWithBaseline(const std::string& filename,
                                const std::string& currentHash,
                                float currentRms,
                                float currentPeak,
                                float rmsTolerance = 1e-4f,
                                float peakTolerance = 1e-4f)
{
    namespace fs = std::filesystem;
    fs::path inPath(filename);

    if (!fs::exists(inPath))
    {
        std::cerr << "[compareWithBaseline] Missing baseline file: "
                  << fs::absolute(inPath) << "\n";
        return false;
    }

    std::ifstream inFile(filename);
    if (!inFile.is_open())
    {
        std::cerr << "[compareWithBaseline] Failed to open: "
                  << fs::absolute(inPath) << "\n";
        return false;
    }

    nlohmann::json j;
    inFile >> j;
    inFile.close();

    std::string refHash = j.value("hash", "");
    float refRms  = j.value("rms", 0.0f);
    float refPeak = j.value("peak", 0.0f);

    bool hashOk = (refHash == currentHash);
    bool rmsOk  = std::fabs(refRms - currentRms) < rmsTolerance;
    bool peakOk = std::fabs(refPeak - currentPeak) < peakTolerance;

    if (!hashOk || !rmsOk || !peakOk)
    {
        std::cerr << "[compareWithBaseline] mismatch:\n"
                  << "  hash: " << (hashOk ? "OK" : "DIFF") << "\n"
                  << "  rms:  " << refRms << " vs " << currentRms << "\n"
                  << "  peak: " << refPeak << " vs " << currentPeak << "\n";
        return false;
    }

    std::cout << "[compareWithBaseline] Match confirmed for "
              << fs::absolute(inPath) << "\n";
    return true;
}
