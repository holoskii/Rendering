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

AreaLight::AreaLight()
{
	type = LightType::AreaLight;
}

void AreaLight::setPoints()
{
	if (pointsCreated)
		return;
	Vec3f anglePos = pos - (i / 2.0f) - (j / 2.0f);
	pointsCreated = true;
	if (samples > 1) {
		for (int ii = 0; ii < samples; ii++) {
			for (int jj = 0; jj < samples; jj++) {
				points.push_back(anglePos + (i * (((float)ii) / (samples - 1))) + (j * (((float)jj) / (samples - 1))));
			}
		}
	}
	else {
		points.push_back(pos);
	}
}

void AreaLight::illuminate(const Vec3f& point, Vec3f& lightDir, Vec3f& lightIntensity, float& distance) const
{
	std::cout << "Area light illuminate, error\n";
}
