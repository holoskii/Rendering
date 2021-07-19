#pragma once

class Options;
class Scene;
struct IntersectInfo;

#include "geometry.h"
#include "objects.h"
#include "lights.h"
#include "mesh.h"
#include "options.h"

enum class RayType { PrimaryRay, ShadowRay };

class Scene
{
public:
	bool sceneLoadSuccess = true;
	
	ObjectVector objects;
	LightsVector lights;
	Options options;

	std::atomic<int> finishedPixels{ 0 };
	std::atomic<int> finishedWorkers{ 0 };

	Scene(const std::string& sceneName);
	int render();
	int launchWorkers(Vec3f* frameBuffer);
	void renderWorker(Vec3f* frameBuffer, size_t y0, size_t y1);
};

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

	static bool trace(const Vec3f& orig, const Vec3f& dir, const ObjectVector& objects,
		IntersectInfo& intrInfo, const RayType rayType = RayType::PrimaryRay);

	static Vec3f castRay(const Vec3f& orig, const Vec3f& dir, const ObjectVector& objects, 
		const LightsVector& lights, const Options& options, const int depth);
};