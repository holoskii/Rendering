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

	int minBatchSizeAccelStruct = 10;
	int maxDepthAccelStruct = 5;
};


namespace options
{
	inline bool useAreaLightAcceleration = 1;

	inline Options globalOptions;
	inline bool useGlobal_width = false;
	inline bool useGlobal_height = false;
	inline bool useGlobal_fov = false;
	inline bool useGlobal_n_workers = false;
	inline bool useGlobal_max_ray_depth = false;
	inline bool useGlobal_ac_max_depth = false;
	inline bool useGlobal_ac_min_batch = false;

	inline void combineWithGlobal(Options& options)
	{
		if (useGlobal_width) options.width = globalOptions.width;
		if (useGlobal_height) options.height = globalOptions.height;
		if (useGlobal_fov) options.fov = globalOptions.fov;
		if (useGlobal_n_workers) options.nWorkers = globalOptions.nWorkers;
		if (useGlobal_max_ray_depth) options.maxRayDepth = globalOptions.maxRayDepth;
		if (useGlobal_ac_max_depth) options.maxDepthAccelStruct = globalOptions.maxDepthAccelStruct;
		if (useGlobal_ac_min_batch) options.minBatchSizeAccelStruct = globalOptions.minBatchSizeAccelStruct;
	}
}