// classes describing light sources
#pragma once

#include <vector>
#include <memory>
class Light;
using LightsVector = std::vector<std::unique_ptr<Light>>;
enum class LightType { BaseLight, DistantLight, PointLight };

#include "geometry.h"
#include "util.h"

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
	DistantLight();
	DistantLight(const Vec3f& a_dir, const Vec3f& a_color = 1, const float& a_intensity = 1);
	void illuminate(const Vec3f& point, Vec3f& lightDir, Vec3f& lightIntensity, float& distance) const;

	Vec3f dir;
};

class PointLight : public Light
{
public:
	PointLight(const Vec3f& a_pos, const Vec3f& a_color = 1, const float& a_intensity = 1);
	void illuminate(const Vec3f& point, Vec3f& lightDir, Vec3f& lightIntensity, float& distance) const;

	Vec3f pos;
};
