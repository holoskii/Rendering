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

class Light
{
public:
	Light(const Vec3f& a_color = 1, const float& a_intensity = 1);
	virtual void illuminate(const Vec3f& point, Vec3f&, Vec3f&, float&) const = 0;

	Vec3f color;
	float intensity;
	LightType type = LightType::BaseLight;
};

class DistantLight : public Light
{
public:
	DistantLight(const Vec3f& a_dir = { 0, 0, -1 }, const Vec3f& a_color = 1, const float& a_intensity = 1);
	void illuminate(const Vec3f& point, Vec3f& lightDir, Vec3f& lightIntensity, float& distance) const;

	Vec3f dir;
};

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

	Vec3f pos;
	Vec3f i;
	Vec3f j;
	int samples = 1;

	bool pointsCreated = false;
	std::vector<Vec3f> points;
};