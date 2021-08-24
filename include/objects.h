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
// Object type are stored in base class
enum class ObjectType { Object, Sphere, Plane, Mesh };
enum class MaterialType { Diffuse, Reflective, Transparent, Phong };

#include "geometry.h"
#include "options.h"

// Base object class. Stores position, type, and surface properties
class Object
{
public:
	Object(const Vec3f& a_center = 1, const Vec3f& a_color = 1,
		const MaterialType& a_materialType = MaterialType::Diffuse);
	virtual ~Object();
	// Checks if ray intersects with object. If it does, return true and  UV coordinate
	virtual bool intersectObject(const Ray& ray, float& t0, Vec2f& uv) const = 0;
	// Gets normal and texture in hit point
	virtual void getSurfaceData(const Vec3f& hitPoint, const Triangle* const triPtr,
		const Vec2f& uv, Vec3f& hitNormal, Vec2f& tex) const = 0;

	ObjectType objectType = ObjectType::Object;

	Vec3f pos;

	Vec3f color;
	MaterialType materialType;
	float indexOfRefraction = 1.4f;
	float ambient = 0.1f;
	float diffuse = 0.1f;
	float specular = 1.0f;
	float nSpecular = 5.0f;
};

class Triangle
{
public:
	Triangle(const Vec3f& a_a, const Vec3f& a_b, const Vec3f& a_c);
	Triangle(const Vec3f& a_a, const Vec3f& a_b, const Vec3f& a_c, 
		const Vec3f& a_n_a, const Vec3f& a_n_b, const Vec3f& a_n_c);
	Triangle(const Vec3f& a_a, const Vec3f& a_b, const Vec3f& a_c, 
		const Vec3f& a_n_a, const Vec3f& a_n_b, const Vec3f& a_n_c,
		const Vec2f& a_t_a, const Vec2f& a_t_b, const Vec2f& a_t_c);
	static bool rayTriangleIntersect(const Ray& ray, const Triangle* triPtr,
		float& t, Vec2f& uv);

	Vec3f a, b, c;			// vertex position
	Vec3f n_a, n_b, n_c;	// normals in vertices
	Vec2f t_a, t_b, t_c;	// texture coordinates

	// tangent and bitangent are calculated just once
	Vec3f tangent, bitangent;
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
		const Vec2f& uv, Vec3f& hitNormal, Vec2f& texCoord) const;

	// Get value from map
	Vec3f getDiffuseColor(const Vec2f& hitTexCoordinates) const;
	float getSpecularValue(const Vec2f& hitTexCoordinates) const;

	// Loading info
	bool loadOBJ(const std::string& filename, const Options& options);
	bool loadDiffuseMap(const std::string& filename);
	bool loadNormalMap(const std::string& filename);
	bool loadSpecularMap(const std::string& filename);

	// Objects are normalized upon loading, such as they fit in size 
	// Proportions are not modified
	Vec3f size;

	// Also, object may be rotated
	Vec3f rot;
	
	// Save all pointers in one place, to avoid double deletion
	std::vector<const Triangle*> allTris;

	// Stores triangle, accelerates intersection
	std::unique_ptr<AccelerationStructure> ac;
	
	// Diffuse map stores color
	bool diffuseMapLoaded = false;
	int diffuseMapWidth = 0;
	int diffuseMapHeight = 0;
	Vec3f* diffuseMap = nullptr;

	// Normal map stores tangent normal
	bool normalMapLoaded = false;
	int normalMapWidth = 0;
	int normalMapHeight = 0;
	Vec3f* normalMap = nullptr;

	// Specular map stores specular coefficient
	bool specularMapLoaded = false;
	int specularMapWidth = 0;
	int specularMapHeight = 0;
	float* specularMap = nullptr;
};

// Acceleration Structure is used to speed up ray-mesh intersection
class AccelerationStructure
{
public:
	AccelerationStructure();
	~AccelerationStructure();

	// Set min and max coordinates
	void setBounds(const Vec3f& a, const Vec3f& b);

	// Create AC tree
	void setup(std::vector<const Triangle*>& a_tris, int a_depth, const Options& options);
	
	// Try intersection
	bool intersectBox(const Ray& ray) const;
	
	// Try intersection with AC mesh
	bool intersectAccelStruct(const Ray& ray, float& t0, const Triangle*& triPtr, Vec2f& uv) const;
	
	// Count intersections AC and sub-AC with ray
	int recCountAC(const Ray& ray);

	// Calculate SAH - Surface Area Heuristic
	static float calculateSAH(const int orientation, const std::vector<const Triangle*>& tris,
		const Vec3f bounds[2], const float boundary);

	// Run binary search to determine optimal split
	static float binarySearchSAH(const int orientation, const std::vector<const Triangle*>& tris,
		const Vec3f bounds[2], const float left, const float right);
	
	// Get coordinate of optimal split
	static float getOptimalSplit(const std::vector<const Triangle*>& tris, const int orientation,
		const Vec3f bounds[2], std::vector<const Triangle*>& trisLeft, std::vector<const Triangle*>& trisRight);

	// Left and Right ancestors
	std::unique_ptr<AccelerationStructure> left;
	std::unique_ptr<AccelerationStructure> right;
	
	// If AC has no ancestors, it has triangles
	std::vector<const Triangle*> tris;
	Vec3f bounds[2];
};

// Sphere primitive
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

// Plane primitive
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
