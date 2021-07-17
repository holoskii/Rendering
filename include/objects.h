// classes describing objects: primitives like sphere and plane and polygon mesh
#pragma once

#include <vector>
#include <memory>
class Object;
class Triangle;
class AccelerationStructure;
using ObjectVector = std::vector<std::unique_ptr<Object>>;
enum class ObjectType { Object, Sphere, Plane, Mesh };
enum class MaterialType { Diffuse, Reflective, Transparent, Phong };
enum class PatternType { None, Stripped, Chessboard, ShadedChessboard };

#include "geometry.h"

class Object
{
public:
	Object(const Vec3f& a_center = 1, const Vec3f& a_color = 1, const MaterialType& a_materialType = MaterialType::Diffuse);
	virtual ~Object();
	float getPattern(Vec2f texture) const;
	virtual bool intersectObject(const Vec3f& orig, const Vec3f& dir, float& t0, Vec2f& uv) const = 0;
	virtual void getSurfaceData(const Vec3f& hitPoint, const Triangle* const triPtr, const Vec2f& uv, Vec3f& hitNormal, Vec2f& tex) const = 0;

	Vec3f center;
	Vec3f color;
	MaterialType materialType;
	ObjectType objectType = ObjectType::Object;
	PatternType pattern = PatternType::None;
	float indexOfRefraction = 1.4f; // used for Reflective only
	float ambient = 0.1f;			// used for Phong only
	float difuse = 0.1f;			// used for Phong only
	float specular = 1.0f;			// used for Phong only
	float nSpecular = 5.0f;         // used for Phong only
};

class Sphere : public Object
{
public:
	Sphere(const Vec3f& a_center = 0, const float a_r = 1, const Vec3f& a_color = 1, const MaterialType& a_materialType = MaterialType::Diffuse);
	bool intersectObject(const Vec3f& orig, const Vec3f& dir, float& t0, Vec2f& uv) const;
	void getSurfaceData(const Vec3f& hitPoint, const Triangle* const triPtr, const Vec2f& uv, Vec3f& hitNormal, Vec2f& tex) const;

	float r;
	float r2;
};

class Plane : public Object
{
public:
	Plane(const Vec3f& a_center = 1, const Vec3f& a_normal = { 0, 1, 0 },
		const Vec3f& a_color = 1, const MaterialType& a_materialType = MaterialType::Diffuse);
	bool intersectObject(const Vec3f& orig, const Vec3f& dir, float& t0, Vec2f& uv) const;
	void getSurfaceData(const Vec3f& hitPoint, const Triangle* const triPtr, const Vec2f& uv,
		Vec3f& hitNormal, Vec2f& tex) const;

	Vec3f normal;
};

class Triangle
{
public:
	Triangle(const Vec3f& a_a, const Vec3f& a_b, const Vec3f& a_c);
	Triangle(const Vec3f& a_a, const Vec3f& a_b, const Vec3f& a_c, const Vec3f& a_n_a, const Vec3f& a_n_b, const Vec3f& a_n_c);
	static bool rayTriangleIntersect(const Vec3f& orig, const Vec3f& dir, const Triangle* triPtr, float& t, Vec2f& uv);

	Vec3f a, b, c;
	Vec3f n_a, n_b, n_c;

	AccelerationStructure* ac = nullptr;
};

class AccelerationStructure
{
public:
	AccelerationStructure();
	~AccelerationStructure();

	void setBounds(const Vec3f& a, const Vec3f& b);
	void setTris(std::vector<const Triangle*>* a_tris);
	bool intersectBox(const Vec3f& orig, const Vec3f& dir) const;
	bool intersectAccelStruct(const Vec3f& orig, const Vec3f& dir, float& t0, const Triangle*& triPtr, Vec2f& uv) const;
	void freeMemory();

	AccelerationStructure* left = nullptr;
	AccelerationStructure* right = nullptr;
	std::vector<const Triangle*>* tris = nullptr;
	int index = 0;
private:
	Vec3f bounds[2];
	
};

class Mesh : public Object
{
public:
	Mesh();
	~Mesh();

	//bool rayTriangleIntersect(const Vec3f& orig, const Vec3f& dir, const Triangle& tri, float& t, Vec2f& uv) const;
	bool intersectObject(const Vec3f& orig, const Vec3f& dir, float& t0, Vec2f& uv) const;
	bool intersectMesh(const Vec3f& orig, const Vec3f& dir, float& t0, const Triangle*& triPtr, Vec2f& uv) const;
	void getSurfaceData(const Vec3f& hitPoint, const Triangle* const triPtr, const Vec2f& uv, Vec3f& hitNormal, Vec2f& tex) const;

	AccelerationStructure* accelStruct = nullptr;
};
