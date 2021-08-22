// classes describing object primitives, such as sphere and plane
#pragma once

#include <vector>
#include <memory>

class Object;
class Mesh;
class AccelerationStructure;
class Triangle;
class Sphere;
class Plane;
class Triangle;

using ObjectVector = std::vector<std::unique_ptr<Object>>;
enum class ObjectType { Object, Sphere, Plane, Mesh };
enum class MaterialType { Diffuse, Reflective, Transparent, Phong };

#include "geometry.h"
#include "options.h"

class Object
{
public:
	Object(const Vec3f& a_center = 1, const Vec3f& a_color = 1,
		const MaterialType& a_materialType = MaterialType::Diffuse);
	virtual ~Object();
	virtual bool intersectObject(const Ray& ray, float& t0, Vec2f& uv) const = 0;
	virtual void getSurfaceData(const Vec3f& hitPoint, const Triangle* const triPtr,
		const Vec2f& uv, Vec3f& hitNormal, Vec2f& tex) const = 0;

	Vec3f pos;
	Vec3f color;
	MaterialType materialType;
	ObjectType objectType = ObjectType::Object;
	float indexOfRefraction = 1.4f; // used for Transparent only
	float ambient = 0.1f;			// used for Phong only
	float difuse = 0.1f;			// used for Phong only
	float specular = 1.0f;			// used for Phong only
	float nSpecular = 5.0f;         // used for Phong only
};

class Triangle
{
public:
	Triangle(const Vec3f& a_a, const Vec3f& a_b, const Vec3f& a_c);
	Triangle(const Vec3f& a_a, const Vec3f& a_b, const Vec3f& a_c, const Vec3f& a_n_a,
		const Vec3f& a_n_b, const Vec3f& a_n_c);
	static bool rayTriangleIntersect(const Ray& ray, const Triangle* triPtr,
		float& t, Vec2f& uv);

	Vec3f a, b, c;
	Vec3f n_a, n_b, n_c;
};

class Mesh : public Object
{
public:
	Mesh();
	~Mesh();

	bool intersectObject(const Ray& ray, float& t0, Vec2f& uv) const;
	bool intersectMesh(const Ray& ray, float& t0, const Triangle*& triPtr,
		Vec2f& uv) const;
	void getSurfaceData(const Vec3f& hitPoint, const Triangle* const triPtr,
		const Vec2f& uv, Vec3f& hitNormal, Vec2f& tex) const;
	bool loadOBJ(const std::string& filename, const Options& options);


	Vec3f size;
	Vec3f rot;
	std::vector<const Triangle*> allTris;
	std::unique_ptr<AccelerationStructure> ac;
};

class AccelerationStructure
{
public:
	AccelerationStructure();
	~AccelerationStructure();

	void setBounds(const Vec3f& a, const Vec3f& b);
	void setup(std::vector<const Triangle*>& a_tris, int a_depth, const Options& options);
	bool intersectBox(const Ray& ray) const;
	bool intersectAccelStruct(const Ray& ray, float& t0, const Triangle*& triPtr, Vec2f& uv) const;
	int recCountAC(const Ray& ray);

	// SAH - Surface Area Heuristic
	static float calculateSAH(const int orientation, const std::vector<const Triangle*>& tris,
		const Vec3f bounds[2], const float boundary);
	static float binarySearchSAH(const int orientation, const std::vector<const Triangle*>& tris,
		const Vec3f bounds[2], const float left, const float right);
	static float getOptimalSplit(const std::vector<const Triangle*>& tris, const int orientation,
		const Vec3f bounds[2], std::vector<const Triangle*>& trisLeft, std::vector<const Triangle*>& trisRight);

	std::unique_ptr<AccelerationStructure> left;
	std::unique_ptr<AccelerationStructure> right;
	std::vector<const Triangle*> tris;
	Vec3f bounds[2];
};


class Sphere : public Object
{
public:
	Sphere(const Vec3f& a_center = 0, const float a_r = 1, const Vec3f& a_color = 1,
		const MaterialType& a_materialType = MaterialType::Diffuse);
	bool intersectObject(const Ray& ray, float& t0, Vec2f& uv) const;
	void getSurfaceData(const Vec3f& hitPoint, const Triangle* const triPtr, const Vec2f& uv,
		Vec3f& hitNormal, Vec2f& tex) const;

	float r;
	float r2;
};

class Plane : public Object
{
public:
	Plane(const Vec3f& a_center = 1, const Vec3f& a_normal = { 0, 1, 0 },
		const Vec3f& a_color = 1, const MaterialType& a_materialType = MaterialType::Diffuse);
	bool intersectObject(const Ray& ray, float& t0, Vec2f& uv) const;
	void getSurfaceData(const Vec3f& hitPoint, const Triangle* const triPtr, const Vec2f& uv,
		Vec3f& hitNormal, Vec2f& tex) const;

	Vec3f normal;
};
