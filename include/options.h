// Class describing options, and namespace for global setting
#pragma once

#include <filesystem>
#include <string>

#include "geometry.h"

class Options
{
public:
	size_t width = 800, height = 600;		// screen dimensions in pixels
	float bias = 0.0001f;	// bias is used to avoid self-intersections
	int maxRayDepth = 10;
	int nWorkers = 32;
	Vec3f backgroundColor { 0.0f, 0.0f, 0.0f };
	int acPenalty = 1;	// determines amount of acceleration structures
	char skyboxNames[6][64] = { { 0 } };	// skybox names
	std::string imageName = "out";
};


namespace options
{
	// global settings
	inline bool outputProgress			= true;
	inline bool useBackfaceCulling		= true;
	inline bool collectStatistics		= false;
	inline bool enableOutput			= true;
	inline bool imageOutput				= true;
	inline bool useAC					= true;
	inline bool showAC					= false;
	inline bool useSkybox				= false;
	inline bool useTextures				= true;
	inline bool showNormals				= false;
}
