#pragma once
#include <cstdint>
#include <cstdio>
#include <string>
#include <sys/stat.h>

namespace logutil {

// Rotate if file exceeds 1 MiB (1 048 576 bytes)
inline constexpr std::uintmax_t kRotateBytes = (1u << 20);

inline void rotateIfLarge(const char* path,
                          std::uintmax_t thresholdBytes = kRotateBytes)
{
    struct stat st{};
    if (::stat(path, &st) == 0 && static_cast<std::uintmax_t>(st.st_size) >= thresholdBytes)
    {
        std::string bak = std::string(path) + ".1";
        std::remove(bak.c_str());
        std::rename(path, bak.c_str());
    }
}

} // namespace logutil
