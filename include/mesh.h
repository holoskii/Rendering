// classes describing polygon mesh, single polygon and acceleration structure
#pragma once

class Mesh;
class AccelerationStructure;
class Triangle;

#include <vector>
#include "objects.h"
#include "geometry.h"
#include "options.h"
#include "renderer.h"

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

	bool intersectObject(const Vec3f& orig, const Vec3f& dir, float& t0, Vec2f& uv) const;
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
	
	static float calculateSAH(const int orientation, const std::vector<const Triangle*>& tris,
		const Vec3f bounds[2], const float boundary);
	static float binarySearchSAH(const int orientation, const std::vector<const Triangle*>& tris,
		const Vec3f bounds[2], const float left, const float right);
	static float getOptimalSplit(const std::vector<const Triangle*>& tris, const int orientation,
		const Vec3f bounds[2], std::vector<const Triangle*>& trisLeft, std::vector<const Triangle*>& trisRight);
	static int countAC(const Ray& ray, const Scene& scene);

	std::unique_ptr<AccelerationStructure> left;
	std::unique_ptr<AccelerationStructure> right;
	std::vector<const Triangle*> tris;
	Vec3f bounds[2];
};
