#pragma once

#include "timer.h"
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
#include <map>
#include <limits>
#include <thread>
#include <chrono>
#include <cassert>
#include <string>
#include <string_view>
#include <sstream>

class Options 
{
public:
    int width = 800, height = 600;
    float fov = 90;
    float zNear = 0.1, zFar = 100;
    int maxDepth = 5;
    float bias = 0.0001;
    std::string imagePath = "D:\\dev\\RayTracing";
    std::string imageName = "out.ppm";
    Vec3f backgroundColor{ 0.2, 0.2, 0.2 };
    int nWorkers = 8;
    
    Options() {}

    
};

