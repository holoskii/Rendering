#include "objects.h"
#include "timer.h"
#include "util.h"
#include "renderer.h"

Object::Object(const Vec3f& a_center, const Vec3f& a_color, const MaterialType& a_materialType)
	: color(a_color), center(a_center), materialType(a_materialType) {}

Object::~Object() {}

float Object::getPattern(Vec2f texture) const
{
	if (pattern == PatternType::None)
		return 1.0f;
	float scaleS = 10, scaleT = 10;
	float angle = degToRad(45);
	float s = texture.x * cos(angle) - texture.y * sin(angle);
	float t = texture.y * cos(angle) + texture.x * sin(angle);
	float res = 1.0f;

	if (pattern == PatternType::Stripped)
		res = (modulo(s * scaleS) < 0.5);
	else if (pattern == PatternType::Chessboard)
		res = (float)((modulo(s * scaleS) < 0.5) ^ (modulo(t * scaleT) < 0.5));
	else if (pattern == PatternType::ShadedChessboard)
		res = (cos(texture.y * 2 * M_PI * scaleT * t) * sin(texture.x * 2 * M_PI * scaleS * s) + 1) * 0.5f;
	return res < 0.1f ? 0.1f : res;
}


Sphere::Sphere(const Vec3f& a_center, const float a_r, const Vec3f& a_color,
	const MaterialType& a_materialType)
	: Object(a_center, a_color, a_materialType), r(a_r)
{
	r2 = r * r;
	objectType = ObjectType::Sphere;
}

bool Sphere::intersectObject(const Vec3f& orig, const Vec3f& dir, float& t0, Vec2f& uv) const
{
	Vec3f L = center - orig;
	float tca = L.dotProduct(dir);
	float d2 = L.dotProduct(L) - tca * tca;
	if (d2 > r2) return false;
	float thc = sqrtf(r2 - d2);
	t0 = tca - thc;
	float t1 = tca + thc;
	if (t0 < 0) t0 = t1;
	if (t0 < 0) return false;
	return true;
}

void Sphere::getSurfaceData(const Vec3f& hitPoint, const Triangle* const triPtr, const Vec2f& uv, 
	Vec3f& hitNormal, Vec2f& tex) const
{
	hitNormal = hitPoint - center;
	hitNormal.normalize();

	tex.x = (1.0f + atan2(hitNormal.z, hitNormal.x) / M_PI) * 0.5f;
	tex.y = acosf(hitNormal.y) / M_PI;
}


Plane::Plane(const Vec3f& a_center, const Vec3f& a_normal, const Vec3f& a_color, 
	const MaterialType& a_materialType)
	: Object(a_center, a_color, a_materialType), normal(a_normal)
{
	normal.normalize();
	objectType = ObjectType::Plane;
}

bool Plane::intersectObject(const Vec3f& orig, const Vec3f& dir, float& t0, Vec2f& uv) const
{
	float denom = dir.dotProduct(normal);
	if (fabs(denom) < 1e-8)
		return false;
	t0 = ((center - orig).dotProduct(normal)) / denom;
	return (t0 >= 0);
}

void Plane::getSurfaceData(const Vec3f& hitPoint, const Triangle* const triPtr, const Vec2f& uv, 
	Vec3f& hitNormal, Vec2f& tex) const
{
	hitNormal = normal;

	Vec3f dist = hitPoint - center;
	tex.x = dist.x / 15;
	tex.y = dist.z / 15;
}
