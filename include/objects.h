#pragma once

class Sphere;

#include "geometry.h"

#ifndef M_PI
#define M_PI	3.14159265358979323846
#endif

enum class MaterialType { Diffuse, Reflective, Transparent };

class Object
{
public:
    Object(const Vec3f& a_center = 1, const Vec3f& a_color = 1, 
        const MaterialType& a_materialType = MaterialType::Diffuse)
        : color(a_color), center(a_center), materialType(a_materialType) {}
    virtual bool intersect(const Vec3f& orig, const Vec3f& dir, float& t0) const = 0;
    virtual void getSurfaceData(const Vec3f& Phit, Vec3f& Nhit, Vec2f& tex) const = 0;

    Vec3f center;
    Vec3f color;
    MaterialType materialType;
    float indexOfRefraction = 1.4;
};

class Sphere : public Object
{
public:
    Sphere(const Vec3f& a_center = 0, const float a_r = 1, const Vec3f& a_color = 1, 
        const MaterialType& a_materialType = MaterialType::Diffuse)
        : Object(a_center, a_color, a_materialType), r(a_r)
    {
        r2 = r * r;
    }

    bool intersect(const Vec3f& orig, const Vec3f& dir, float& t0) const 
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

    void getSurfaceData(const Vec3f& hitPoint, Vec3f& hitNormal, Vec2f& tex) const
    {
        hitNormal = hitPoint - center;
        hitNormal.normalize();

        tex.x = (1 + atan2(hitNormal.z, hitNormal.x) / M_PI) * 0.5;
        tex.y = acosf(hitNormal.y) / M_PI;
    }

    float r;
    float r2;
};

class Plane : public Object
{
public:
    Plane(const Vec3f& a_center = 1, const Vec3f& a_normal = { 0, 1, 0 }, const Vec3f& a_color = 1, 
        const MaterialType& a_materialType = MaterialType::Diffuse)
        : Object(a_center, a_color, a_materialType), normal(a_normal)
    {
        normal.normalize();
    }

    bool intersect(const Vec3f& orig, const Vec3f& dir, float& t) const 
    {
        float denom = dir.dotProduct(normal);
        if (fabs(denom) < 1e-6)
            return false;
        t = ((center - orig).dotProduct(normal)) / denom;
        return (t >= 0);
    }

    void getSurfaceData(const Vec3f& hitPoint, Vec3f& hitNormal, Vec2f& tex) const
    {
        hitNormal = normal;

        Vec3f dist = hitPoint - center;
        tex.x = dist.x / 15;
        tex.y = dist.z / 15;
    }

    Vec3f normal;
};