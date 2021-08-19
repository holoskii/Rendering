// classed describing scene and camera
#pragma once

struct IntersectInfo;
class Render;
class Camera;
class Scene;

#include <atomic>

#include "geometry.h"
#include "objects.h"
#include "lights.h"
#include "options.h"

struct IntersectInfo
{
	const Object* hitObject = nullptr;
	float tNear = std::numeric_limits<float>::max();
	const Triangle* triPtr = nullptr;
	Vec2f uv{ -1,-1 };
};

class Render
{
public:
	static Vec3f reflect(const Vec3f& dir, const Vec3f& normal);
	static Vec3f refract(const Vec3f& dir, const Vec3f& normal, const float& indexOfRefraction);
	static float fresnel(const Vec3f& dir, const Vec3f& normal, const float& indexOfRefraction);

	static bool trace(const Ray& ray, const ObjectVector& objects, IntersectInfo& intrInfo);

	static Vec3f castRay(const Ray& ray, const Scene& scene, const int depth);
};

class Camera
{
public:
	Vec3f pos;
	Vec3f rot;
	Matrix44f rMatrix;

	Camera(const Vec3f& a_pos = { 0, 0, 0 }, const Vec3f& a_rot = { 0, 0, 0 });
	Ray getRay(const float xPix, const  float yPix);
};

class Scene
{
public:
	bool sceneLoadSuccess = true;

	ObjectVector objects;
	LightsVector lights;
	Options options;
	Camera camera;

	std::atomic<int> finishedPixels{ 0 };
	std::atomic<int> finishedWorkers{ 0 };

	Scene(const std::string& sceneName);
	bool loadScene(const std::string& sceneName);

	long long render();
	int launchWorkers(Vec3f* frameBuffer);
	void renderWorker(Vec3f* frameBuffer, size_t y0, size_t y1);

	int countAC(const Ray& ray);
};