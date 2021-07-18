#include "mesh.h"
#include "renderer.h"

#include <cassert>

Mesh::Mesh()
{
	objectType = ObjectType::Mesh;
}

Mesh::~Mesh()
{
	if (accelStruct) delete accelStruct;
	for (const Triangle* tri : allTris)
		delete tri;
}

bool Mesh::intersectObject(const Vec3f& orig, const Vec3f& dir, float& t0, Vec2f& uv) const
{
	std::cout << "Object intersect called with mesh\n";
	std::exit(-1);
}

bool Mesh::intersectMesh(const Vec3f& orig, const Vec3f& dir, float& t0, const Triangle*& triPtr, Vec2f& uv) const
{
	return accelStruct->intersectAccelStruct(orig, dir, t0, triPtr, uv);
}

void Mesh::getSurfaceData(const Vec3f& hitPoint, const Triangle* const triPtr, const Vec2f& uv, Vec3f& hitNormal, Vec2f& tex) const
{
#if 1
	// vertex normal shading
	hitNormal = ((triPtr->n_b * uv.x + triPtr->n_c * uv.y + triPtr->n_a * (1 - uv.x - uv.y)) / 3).normalize();
#else
	// triangle normal shading
	const Vec3f& v0 = triPtr->a;
	const Vec3f& v1 = triPtr->b;
	const Vec3f& v2 = triPtr->c;
	hitNormal = (v1 - v0).crossProduct(v2 - v0).normalize();
#endif
	tex = Vec2f{ uv.x, uv.y };
}


AccelerationStructure::AccelerationStructure()
{
	static int globalIndex = 0;
	index = globalIndex++;
}

AccelerationStructure::~AccelerationStructure()
{
	if (tris != nullptr) delete tris;
	if (left != nullptr) delete left;
	if (right != nullptr) delete right;
}

void AccelerationStructure::setTris(std::vector<const Triangle*>* a_tris)
{
	if (a_tris->size() < 10) {
#ifdef _STATS
		triCopiesCount.store(triCopiesCount.load() + a_tris->size());
#endif // _STATS
		tris = a_tris;
		return;
	}

	int split = 1; // xyz split
	Vec3f dim = bounds[1] - bounds[0];
	if (dim.x > dim.y && dim.x > dim.z)
		split = 0;
	else if (dim.y > dim.z)
		split = 1;
	else
		split = 2;

	float avg = 0.0f;
	std::vector<const Triangle*>* trisLeft = new std::vector<const Triangle*>;
	std::vector<const Triangle*>* trisRight = new std::vector<const Triangle*>;

	if (split == 0) {
		for (const Triangle* const tri : *a_tris)
			avg += tri->a.x + tri->b.x + tri->c.x;
		avg /= 3.0f * a_tris->size();
		for (const Triangle* const tri : *a_tris) {
			if (tri->a.x <= avg || tri->b.x <= avg || tri->c.x <= avg) 
				trisLeft->push_back(tri);
			if (tri->a.x >= avg || tri->b.x >= avg || tri->c.x >= avg) 
				trisRight->push_back(tri);
		}
		left = new AccelerationStructure();
		left->setBounds(bounds[0], Vec3f{ avg, bounds[1].y, bounds[1].z });
		left->setTris(trisLeft);
		right = new AccelerationStructure();
		right->setBounds(Vec3f{ avg, bounds[0].y, bounds[0].z }, bounds[1]);
		right->setTris(trisRight);
	}
	else if (split == 1) {
		for (const Triangle* tri : *a_tris)
			avg += tri->a.y + tri->b.y + tri->c.y;
		avg /= 3.0f * a_tris->size();
		for (const Triangle* tri : *a_tris) {
			if (tri->a.y <= avg || tri->b.y <= avg || tri->c.y <= avg)
				trisLeft->push_back(tri);
			if (tri->a.y >= avg || tri->b.y >= avg || tri->c.y >= avg)
				trisRight->push_back(tri);
		}
		left = new AccelerationStructure();
		left->setBounds(bounds[0], Vec3f{ bounds[1].x, avg, bounds[1].z });
		left->setTris(trisLeft);
		right = new AccelerationStructure();
		right->setBounds(Vec3f{ bounds[0].x, avg, bounds[0].z }, bounds[1]);
		right->setTris(trisRight);
	}
	else if (split == 2) {
		for (const Triangle* tri : *a_tris)
			avg += tri->a.z + tri->b.z + tri->c.z;
		avg /= 3.0f * a_tris->size();
		for (const Triangle* tri : *a_tris) {
			if (tri->a.z <= avg || tri->b.z <= avg || tri->c.z <= avg)
				trisLeft->push_back(tri);
			if (tri->a.z >= avg || tri->b.z >= avg || tri->c.z >= avg)
				trisRight->push_back(tri);
		}
		left = new AccelerationStructure();
		left->setBounds(bounds[0], Vec3f{ bounds[1].x, bounds[1].y, avg });
		left->setTris(trisLeft);
		right = new AccelerationStructure();
		right->setBounds(Vec3f{ bounds[0].x, bounds[0].y, avg }, bounds[1]);
		right->setTris(trisRight);
	}
	else {
		assert(false);
	}

	
	delete a_tris;
}

void AccelerationStructure::setBounds(const Vec3f& a, const Vec3f& b)
{
	bounds[0] = a;
	bounds[1] = b;
}

bool AccelerationStructure::intersectBox(const Vec3f& orig, const Vec3f& dir) const
{
#ifdef _NO_ACCEL_STRUCT
	return true;
#endif // _NO_ACCEL_STRUCT
#ifdef _STATS
	accelStructTests.store(accelStructTests.load() + 1);
#endif // _STATS
	const Vec3f invdir = 1 / dir;
	const int sign[3] = { (invdir.x < 0), (invdir.y < 0), (invdir.z < 0) };

	float tmin, tmax, tymin, tymax, tzmin, tzmax;
	tmin = (bounds[sign[0]].x - orig.x) * invdir.x;
	tmax = (bounds[1 - sign[0]].x - orig.x) * invdir.x;
	tymin = (bounds[sign[1]].y - orig.y) * invdir.y;
	tymax = (bounds[1 - sign[1]].y - orig.y) * invdir.y;

	if ((tmin > tymax) || (tymin > tmax))
		return false;
	if (tymin > tmin)
		tmin = tymin;
	if (tymax < tmax)
		tmax = tymax;

	tzmin = (bounds[sign[2]].z - orig.z) * invdir.z;
	tzmax = (bounds[1 - sign[2]].z - orig.z) * invdir.z;

	if ((tmin > tzmax) || (tzmin > tmax))
		return false;
	if (tzmin > tmin)
		tmin = tzmin;
	if (tzmax < tmax)
		tmax = tzmax;

	return true;
}

bool AccelerationStructure::intersectAccelStruct(const Vec3f& orig, const Vec3f& dir, float& t0, const Triangle*& triPtr, Vec2f& uv) const
{
	if (!this->intersectBox(orig, dir))
		return false;

	bool inter = false;
	float tempT;
	Vec2f tempUV;
	const Triangle* tempTriPtr;
	t0 = std::numeric_limits<float>::max();
	if (left != nullptr) {
		assert(right != nullptr);
		if (left->intersectAccelStruct(orig, dir, tempT, tempTriPtr, tempUV) && tempT < t0) {
			inter = true;
			t0 = tempT;
			uv = tempUV;
			triPtr = tempTriPtr;
		}
		if (right->intersectAccelStruct(orig, dir, tempT, tempTriPtr, tempUV) && tempT < t0) {
			inter = true;
			t0 = tempT;
			uv = tempUV;
			triPtr = tempTriPtr;
		}


		return inter;
	}

	for (const Triangle* tri : *tris) {
		if (Triangle::rayTriangleIntersect(orig, dir, tri, tempT, tempUV) && tempT < t0) {
			inter = true;
			t0 = tempT;
			uv = tempUV;
			triPtr = tri;
		}
	}
	return inter;
}
