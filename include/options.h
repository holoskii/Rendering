// class describing options, and namespace for global functions
#pragma once

#include <string>

#include "geometry.h"

class Options
{
public:
	size_t width = 800, height = 600;
	float bias = 0.0001f;
	int maxRayDepth = 5;
	int nWorkers = 8;

	
	Vec3f backgroundColor{ 0.0f, 0.0f, 0.0f };

	int acPenalty = 1;

	std::string rootPath = "D:\\dev\\CG\\RayTracing";
	std::string imageName = "out";

	char names[6][64];
};


namespace options
{
	// global settings are stored here
	// keeping them constexpr is better for performance
	constexpr bool outputProgress			= 1;
	constexpr bool useBackfaceCulling		= 0;
	constexpr bool collectStatistics		= 0;
	constexpr bool enableOutput				= 1;
	constexpr bool imageOutput				= 1;
	constexpr bool useAC					= 1;
	constexpr bool showAC					= 1;
	constexpr bool useSkybox				= 1;
}