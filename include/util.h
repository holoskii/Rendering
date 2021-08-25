// utility functions
#pragma once

#define _USE_MATH_DEFINES
#include <math.h>
#include <iostream>
#include <vector>
#include <sstream>

#include "geometry.h"
#include "options.h"

#define LOG_ERROR logError(__FILE__, __FUNCTION__, __LINE__);

inline void logError(const char* file, const char* func, int line)
{
	std::cout << "Error: " << file << ' ' << func << ' ' << line << '\n';
}

inline float modulo(const float& f)
{
	return f - std::floor(f);
}

inline float clamp(const float low, const float high, const float val)
{
	return std::max(low, std::min(high, val));
}

inline float degToRad(const float& f)
{
	return f * (float)(M_PI) / 180.0f;
}

inline float radToDeg(const float& f)
{
	return f * (180.0f / (float)(M_PI));
}

inline bool strToBool(const std::string_view& str)
{
	bool result = 0;
	std::stringstream ss{ std::string(str) };
	ss >> result;
	if (!ss.eof() && !ss.good()) LOG_ERROR
		return result;
}

inline int strToInt(const std::string_view& str)
{
	int result = 0;
	std::stringstream ss{ std::string(str) };
	ss >> result;
	if (!ss.eof() && !ss.good()) LOG_ERROR
	return result;
}

inline float strToFloat(const std::string_view& str)
{
	float result = 0;
	std::stringstream ss{ std::string(str) };
	ss >> result;
	if (!ss.eof() && !ss.good()) LOG_ERROR
	return result;
}

inline Vec3f str3ToFloat(std::vector<std::string> vec)
{
	if (vec.size() != 3) LOG_ERROR
	return Vec3f{ strToFloat(vec[0]), strToFloat(vec[1]), strToFloat(vec[2]) };
}

inline std::vector<std::string> splitString(const std::string_view& str, const char delim)
{
	std::vector<std::string> res;
	std::stringstream lineStream{ std::string(str) };
	std::string cell;
	while (std::getline(lineStream, cell, delim))
		res.push_back(cell);
	return res;
}

inline bool strContains(const std::string_view& str, const std::string_view& substr)
{
	return str.find(substr) != std::string::npos;
};

inline bool strEquals(const std::string_view& str1, const std::string_view& str2)
{
	return str1.compare(str2) == 0;
};

int saveImage(Vec3f* frameBuffer, const Options& options);

unsigned char* loadBMP(const char* filename, int& width, int& height);
