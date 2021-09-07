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

// Store all intersect info in one structure to reduce number of parameters
struct IntersectInfo
{
	const Object* hitObject = nullptr;
	float tNear = std::numeric_limits<float>::max();
	const Triangle* triPtr = nullptr;
	Vec2f uv{ -1,-1 };
};

// Some static functions
class Render
{
public:
	// Get reflected ray
	static Vec3f reflect(const Vec3f& dir, const Vec3f& normal);
	
	// Get refracted ray
	static Vec3f refract(const Vec3f& dir, const Vec3f& normal, const float& indexOfRefraction);
	
	// Get Fresnel coefficient
	static float fresnel(const Vec3f& dir, const Vec3f& normal, const float& indexOfRefraction);

	// Check if anything intersects with the ray
	static bool trace(const Ray& ray, const ObjectVector& objects, IntersectInfo& intrInfo);

	// Cast ray
	static Vec3f castRay(const Ray& ray, const Scene& scene, const int depth);
};

// Stores all camera info
class Camera
{
public:
	Vec3f pos;
	Vec3f rot;
	Matrix44f rMatrix;

	float fov = 60.0f;
	float zNear = 0.1f, zFar = 100.0f;

	Camera(const Vec3f& a_pos = { 0, 0, 0 }, const Vec3f& a_rot = { 0, 0, 0 });
	Ray getRay(const float xPix, const  float yPix);

	bool cameraRotated = false;
};

// Stores scene info
class Scene
{
public:
	bool sceneLoadSuccess = true;

	ObjectVector objects;
	LightsVector lights;
	Options options;
	Camera camera;

	// Skybox info
	int skyboxWidth, skyboxHeight;
	Vec3f* skyboxes[6] = { nullptr };

	// Info for statistics
	std::atomic<int> finishedPixels = 0;
	std::atomic<int> runningWorkers = 0;

	Scene(const std::string& sceneName);
	bool loadScene(const std::string& sceneName);
	void loadSkybox();
	Vec3f getSkybox(const Vec3f& dir) const;

	void render();
	void launchWorkers(Vec3f* frameBuffer);
	void renderWorker(Vec3f* frameBuffer, size_t x0, size_t x1, size_t y0, size_t y1);

	int countAC(const Ray& ray);
};
