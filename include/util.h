#pragma once

#include <cmath>
#include <sstream>
#include <cassert>

#include "geometry.h"
#include "renderer.h"
#include "mesh.h"

constexpr float M_PI = 3.14159265358979323846f;

inline float modulo(const float& f)
{
	return f - std::floor(f);
}

inline float degToRad(const float& f)
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

Mesh* loadOBJ(const std::string& filename, const Vec3f& pos, const Vec3f& size, const Options& options);

bool loadScene(Scene& scene, Options& options, const std::string& sceneName);

int recInterAC(const Ray& ray, AccelerationStructure* ac);

int interAC(const Vec3f& orig, const Vec3f& dir, const ObjectVector& objects, const Options& options);
