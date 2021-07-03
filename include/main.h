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
    int width = 800, height = 600;
    float fov = 90;
    float zNear = 0.1, zFar = 100;
    int maxDepth = 5;
    float bias = 0.0001;
    std::string imagePath = "D:\\dev\\RayTracing";
    std::string imageName = "out.ppm";
    Vec3f backgroundColor{ 0.2, 0.2, 0.2 };

    Options() {}

    float getPattern(Vec2f texture) const
    {
        auto modulo = [](const float& f)
        {
            return f - std::floor(f);
        };

        

        float scaleS = 10, scaleT = 10;
        float angle = 45;
        angle *= M_PI / 180.0;

        float s = texture.x * cos(angle) - texture.y * sin(angle);
        float t = texture.y * cos(angle) + texture.x * sin(angle);
        
        //float pattern = (cos(texture.y * 2 * M_PI * scaleT * t) * sin(texture.x * 2 * M_PI * scaleS * s) + 1) * 0.5; // shaded chechboard pattern
        float pattern = (modulo(s * scaleS) < 0.5) ^ (modulo(t * scaleT) < 0.5); // chechboard pattern
        //float pattern = (modulo(s * scaleS) < 0.5); // striped pattern
        
        //return 1.0f;
        return pattern < 0.25 ? 0.25 : pattern;
    }
};

