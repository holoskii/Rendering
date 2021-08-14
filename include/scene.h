// classed describing scene and camera
#pragma once

class Camera;
class Scene;

#include <atomic>

#include "geometry.h"
#include "lights.h"
#include "objects.h"
#include "options.h"

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
	long long render();
	int launchWorkers(Vec3f* frameBuffer);
	void renderWorker(Vec3f* frameBuffer, size_t y0, size_t y1);
};