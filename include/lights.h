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

class AreaLight : public Light
{
public:
	AreaLight();
	void setPoints();
	void illuminate(const Vec3f& point, Vec3f& lightDir, Vec3f& lightIntensity, float& distance) const;
	Vec3f getTotalIlluminance(const Vec3f& hitPoint, const Vec3f& hitNormal, const ObjectVector& objects);

	Vec3f pos;
	Vec3f i;
	Vec3f j;
	int samples = 1;
	int base_samples = 1;

	bool pointsCreated = false;
	std::vector<Vec3f> basePoints;
	std::vector<Vec3f> points;
};