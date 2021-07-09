#pragma once

#include "geometry.h"

#include <vector>

enum class MaterialType { Diffuse, Reflective, Transparent, Phong };
enum class PatternType { None, Stripped, Chessboard, ShadedChessboard };

class Object
{
public:
    Object(const Vec3f& a_center = 1, const Vec3f& a_color = 1, 
        const MaterialType& a_materialType = MaterialType::Diffuse);
    float getPattern(Vec2f texture) const;
    virtual bool intersect(const Vec3f& orig, const Vec3f& dir, float& t0, 
        int& triIndex, Vec2f& uv) const = 0;
    virtual void getSurfaceData(const Vec3f& hitPoint, const int triIndex, 
        const Vec2f& uv, Vec3f& hitNormal, Vec2f& tex) const = 0;

    Vec3f center;
    Vec3f color;
    MaterialType materialType;
    PatternType pattern = PatternType::None;
    float indexOfRefraction = 1.4f;  // used for Reflective only
    float ambient   = 0.1f;          // used for Phong only
    float difuse    = 0.1f;          // used for Phong only
    float specular  = 1.0f;          // used for Phong only
    float nSpecular = 5.0f;          // used for Phong only
};

class Sphere : public Object
{
public:
    Sphere(const Vec3f& a_center = 0, const float a_r = 1, const Vec3f& a_color = 1,
        const MaterialType& a_materialType = MaterialType::Diffuse);
    bool intersect(const Vec3f& orig, const Vec3f& dir, 
        float& t0, int& triIndex, Vec2f& uv) const;
    void getSurfaceData(const Vec3f& hitPoint, const int triIndex, 
        const Vec2f& uv, Vec3f& hitNormal, Vec2f& tex) const;

    float r;
    float r2;
};

class Plane : public Object
{
public:
    Plane(const Vec3f& a_center = 1, const Vec3f& a_normal = { 0, 1, 0 }, 
        const Vec3f& a_color = 1, const MaterialType& a_materialType = MaterialType::Diffuse);
    bool intersect(const Vec3f& orig, const Vec3f& dir, 
        float& t0, int& triIndex, Vec2f& uv) const;
    void getSurfaceData(const Vec3f& hitPoint, const int triIndex, const Vec2f& uv, 
        Vec3f& hitNormal, Vec2f& tex) const;

    Vec3f normal;
};

class Triangle
{
public:
    Triangle(const Vec3f& a_a, const Vec3f& a_b, const Vec3f& a_c);

    Vec3f a, b, c;
};

class Mesh : public Object
{
public:
    Mesh(const std::string& filename);
    bool rayTriangleIntersect(const Vec3f& orig, const Vec3f& dir, const Triangle& tri, 
        float& t, Vec2f& uv) const;
    bool intersect(const Vec3f& orig, const Vec3f& dir, 
        float& t0, int& triIndex, Vec2f& uv) const;
    void getSurfaceData(const Vec3f& hitPoint, const int triIndex, const Vec2f& uv, 
        Vec3f& hitNormal, Vec2f& tex) const;
    void parseOBJ(Mesh& mesh, const std::string& filename);

    std::vector<Triangle> tris;
};

using ObjectVector = std::vector<std::unique_ptr<Object>>;

