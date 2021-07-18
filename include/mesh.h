// class storing polygon mesh data, and acceleration structure 
// that speeds up ray-polygon intersection
#pragma once

class Mesh;
class AccelerationStructure;

#include <vector>
#include "objects.h"
#include "geometry.h"

class Mesh : public Object
{
public:
	Mesh();
	~Mesh();

	//bool rayTriangleIntersect(const Vec3f& orig, const Vec3f& dir, const Triangle& tri, float& t, Vec2f& uv) const;
	bool intersectObject(const Vec3f& orig, const Vec3f& dir, float& t0, Vec2f& uv) const;
	bool intersectMesh(const Vec3f& orig, const Vec3f& dir, float& t0, const Triangle*& triPtr, Vec2f& uv) const;
	void getSurfaceData(const Vec3f& hitPoint, const Triangle* const triPtr, const Vec2f& uv, Vec3f& hitNormal, Vec2f& tex) const;

	std::vector<const Triangle*> allTris;
	AccelerationStructure* accelStruct = nullptr;
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

	AccelerationStructure* left = nullptr;
	AccelerationStructure* right = nullptr;
	std::vector<const Triangle*>* tris = nullptr;
	int index = 0;
private:
	Vec3f bounds[2];

};
