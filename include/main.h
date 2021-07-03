#pragma once

#include "geometry.h"
#include "objects.h"
#include "light.h"
#define NOMINMAX
#include "windows.h"
#include "shellapi.h"
#include <cmath>
#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
#include <vector>
#include <limits>

class Options 
{
public:
    int width = 800;
    int height = 600;
    float fov = 90;
    float zNear = 0.1;
    float zFar = 100;
    int maxDepth = 3;
    float bias = 0.0001;
    Vec3f backgroundColor{ 0.2, 0.2, 0.2 };
    std::string imagePath = "D:\\dev\\RayTracing";
    std::string imageName = "out.ppm";
    Options() {}
};

