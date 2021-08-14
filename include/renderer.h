// functions related to render algorithms
#pragma once

struct IntersectInfo;

#include "geometry.h"
#include "objects.h"
#include "lights.h"
#include "mesh.h"
#include "options.h"
#include "scene.h"

struct IntersectInfo
{
	const Object* hitObject = nullptr;
	float tNear = std::numeric_limits<float>::max();
	const Triangle* triPtr = nullptr;
	Vec2f uv{ -1,-1 };
};

class Renderer
{
public:
	static Vec3f reflect(const Vec3f& dir, const Vec3f& normal);
	static Vec3f refract(const Vec3f& dir, const Vec3f& normal, const float& indexOfRefraction);
	static float fresnel(const Vec3f& dir, const Vec3f& normal, const float& indexOfRefraction);

	static bool trace(const Ray& ray, const ObjectVector& objects, IntersectInfo& intrInfo);

	static Vec3f castRay(const Ray& ray, const Scene& scene, const int depth);
};