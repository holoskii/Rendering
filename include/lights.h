// classes describing light sources such as distant and point light
#pragma once

#include <vector>
#include <memory>

class Light;
class DistantLight;
class PointLight;
using LightsVector = std::vector<std::unique_ptr<Light>>;
enum class LightType { BaseLight, DistantLight, PointLight, AreaLight };

class Object;
using ObjectVector = std::vector<std::unique_ptr<Object>>;
#include "geometry.h"
#include "objects.h"

// Base light class. Stores basic info like color and intensity
class Light
{
public:
	// Basic constructor
	Light(const Vec3f& a_color = 1, const float& a_intensity = 1);
	
	// Function that is used to determine light properties such as direction, intensity
	// and distance at given point
	virtual void illuminate(const Vec3f& point, Vec3f& lightDir, Vec3f& lightIntensity, float& distance) const = 0;

	Vec3f color;
	float intensity;
	LightType type = LightType::BaseLight;
};

// Distant light is light that illuminates scene with parallel rays
class DistantLight : public Light
{
public:
	DistantLight(const Vec3f& a_dir = { 0, 0, -1 }, const Vec3f& a_color = 1, const float& a_intensity = 1);
	void illuminate(const Vec3f& point, Vec3f& lightDir, Vec3f& lightIntensity, float& distance) const;

	Vec3f dir;
};

// Point light has position, illuminates all around it
class PointLight : public Light
{
public:
	PointLight(const Vec3f& a_pos = { 0, 0, 0 }, const Vec3f& a_color = 1, const float& a_intensity = 1);
	void illuminate(const Vec3f& point, Vec3f& lightDir, Vec3f& lightIntensity, float& distance) const;

	Vec3f pos;
};

/* The area light is determined by its position, and 2 base vectors, 
 * forming a parallelogram with a known center. Its light properties 
 * are determined by illuminance (from Light base class) and number of samples
 * per one side of the light source (total number is square of that) */
class AreaLight : public Light
{
public:
	AreaLight();
	void setPoints();
	void illuminate(const Vec3f& point, Vec3f& lightDir, Vec3f& lightIntensity, float& distance) const;

	Vec3f pos;	// pos - coordinates of parallelogram center
	Vec3f i;
	Vec3f j;
	int samples = 1;

	// Coordinates of smaller light sources
	bool pointsCreated = false;
	std::vector<Vec3f> points;
};
