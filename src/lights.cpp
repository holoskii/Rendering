// classes describing light sources such as distant and point light
#include "lights.h"

#define _USE_MATH_DEFINES
#include <math.h>

Light::Light(const Vec3f& a_color, const float& a_intensity)
	: color(a_color), intensity(a_intensity) {}


DistantLight::DistantLight(const Vec3f& a_dir, const Vec3f& a_color, const float& a_intensity)
	: Light(a_color, a_intensity), dir(a_dir)
{
	type = LightType::DistantLight;
	dir.normalize();
}

void DistantLight::illuminate(const Vec3f& point, Vec3f& lightDir, Vec3f& lightIntensity, float& distance) const
{
	lightDir = dir;
	lightIntensity = color * intensity;
	distance = std::numeric_limits<float>::max();
}


PointLight::PointLight(const Vec3f& a_pos, const Vec3f& a_color, const float& a_intensity)
	: Light(a_color, a_intensity), pos(a_pos)
{
	type = LightType::PointLight;
}

void PointLight::illuminate(const Vec3f& point, Vec3f& lightDir, Vec3f& lightIntensity, float& distance) const
{
	lightDir = point - pos;
	lightIntensity = color * std::min(1.0f, (float)(intensity / (4 * M_PI * lightDir.length2() / 1000)));
	lightDir.normalize();
	distance = (point - pos).length();
}