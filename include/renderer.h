#pragma once

class Ray;
class Camera;
class Options;
class Scene;
struct IntersectInfo;

#include "geometry.h"
#include "objects.h"
#include "lights.h"
#include "mesh.h"
#include "options.h"

enum class RayType { PrimaryRay, ShadowRay };

class Ray
{
public:
	RayType rayType;
	Vec3f orig;
	Vec3f dir;
	
	Ray(const Vec3f& a_orig = { 0,0,0 }, const Vec3f& a_dir = { 0,0,-1 }, const RayType a_rayType = RayType::PrimaryRay);
};

class Camera
{
public:
	Vec3f pos;
	Vec3f rot;

	Camera(const Vec3f& a_pos = { 0, 0, 0 }, const Vec3f& a_rot = { 0, 0, 0 });
};

class Scene
{
public:
	bool sceneLoadSuccess = true;
	
	ObjectVector objects;
	LightsVector lights;
	Options options;
	// Camera cam;

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

	static bool trace(const Ray& ray, const ObjectVector& objects, IntersectInfo& intrInfo);

	static Vec3f castRay(const Ray& ray, const Scene& scene, const int depth);
};