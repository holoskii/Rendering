// class describing options, and namespace for global functions
#pragma once

#include <string>

#include "geometry.h"

class Options
{
public:
	size_t width = 800, height = 600;
	float fov = 90.0f;
	float zNear = 0.1f, zFar = 100.0f;
	float bias = 0.0001f;
	int maxRayDepth = 5;
	int nWorkers = 8;

	std::string imagePath = "D:\\dev\\CG\\RayTracing";
	std::string imageName = "out";
	Vec3f backgroundColor{ 0.0f, 0.0f, 0.0f };

	int acPenalty = 1;
};


namespace options
{
	// global settings are stored here
	// keeping them constexpr is better for performance
	constexpr bool outputProgress			= 1;
	constexpr bool useAreaLightAcceleration	= 1;
	constexpr bool useBackfaceCulling		= 1;
	constexpr bool collectStatistics		= 1;
	constexpr bool enableOutput				= 1;
	constexpr bool imageOutput				= 1;
	constexpr bool useAC					= 1;
	constexpr bool showAC					= 0;
	constexpr bool useSkybox				= 1;
}