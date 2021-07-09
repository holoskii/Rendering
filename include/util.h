#pragma once

#include "geometry.h"
#include "main.h"
#include <cmath>
#include <sstream>
#include <cassert>

constexpr float M_PI = 3.14159265358979323846f;

inline float modulo (const float& f)
{
    return f - std::floor(f);
}

inline float degToRad (const float& f)
{
    return f * M_PI / 180;
}

inline float clamp(const float low, const float high, const float val)
{
    return std::max(low, std::min(high, val));
}

inline int strToInt(const std::string& str)
{
    int result = 0;
    std::stringstream ss{ str };
    ss >> result;
    assert(ss.eof() || ss.good());
    return result;
}

inline float strToFloat(const std::string& str)
{
    float result = 0;
    std::stringstream ss{ str };
    ss >> result;
    assert(ss.eof() || ss.good());
    return result;
}

int savePPM(Vec3f* frameBuffer, const Options& options);