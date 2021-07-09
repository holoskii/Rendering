#pragma once

#include "objects.h"

enum class RayType { PrimaryRay, ShadowRay };

class Options
{
public:
    size_t width = 800, height = 600;
    float fov = 90.0f;
    float zNear = 0.1f, zFar = 100.0f;
    int maxDepth = 5;
    float bias = 0.0001f;
    std::string imagePath = "D:\\dev\\RayTracing";
    std::string imageName = "out.ppm";
    Vec3f backgroundColor{ 0.2f, 0.2f, 0.2f };
    int nWorkers = 8;

    Options() {}
};

struct IntersectInfo
{
    const Object* hitObject = nullptr;
    float tNear = std::numeric_limits<float>::max();
    int triIndex = -1;
    Vec2f uv{ -1,-1 };
};