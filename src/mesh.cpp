#include "mesh.h"

#include <cassert>
#include <tuple>
#include <queue>

#include "stats.h"
#include "options.h"

Triangle::Triangle(const Vec3f& a_a, const Vec3f& a_b, const Vec3f& a_c)
	: a(a_a), b(a_b), c(a_c)
{
	n_a = n_b = n_c = (a_b - a_a).crossProduct(a_c - a_a);
}

Triangle::Triangle(const Vec3f& a_a, const Vec3f& a_b, const Vec3f& a_c, const Vec3f& a_n_a,
	const Vec3f& a_n_b, const Vec3f& a_n_c)
	: Triangle(a_a, a_b, a_c)
{
	n_a = a_n_a;
	n_b = a_n_b;
	n_c = a_n_c;
}

bool Triangle::rayTriangleIntersect(const Vec3f& orig, const Vec3f& dir, const Triangle* triPtr,
	float& t, Vec2f& uv)
{
#ifdef _STATS
	stats::rayTriTests.store(stats::rayTriTests.load() + 1);
#endif // _STATS
	const Vec3f& v0 = triPtr->a;
	const Vec3f& v1 = triPtr->b;
	const Vec3f& v2 = triPtr->c;
	// Vec3f N = (v1 - v0).crossProduct(v2 - v0); // normal
	
	float u, v;
	Vec3f v0v1 = v1 - v0;
	Vec3f v0v2 = v2 - v0;
	Vec3f pvec = dir.crossProduct(v0v2);
	float det = v0v1.dotProduct(pvec);

#ifdef _BACKFACE_CULLING
	if (det < 1e-8) return false;
#endif // _BACKFACE_CULLING

	if (fabs(det) < 1e-8) return false;

	float invDet = 1 / det;
	Vec3f tvec = orig - v0;
	u = tvec.dotProduct(pvec) * invDet;
	if (u < 0 || u > 1) return false;

	Vec3f qvec = tvec.crossProduct(v0v1);
	v = dir.dotProduct(qvec) * invDet;
	if (v < 0 || u + v > 1) return false;

	t = v0v2.dotProduct(qvec) * invDet;
	if (t < 0) return false;

	uv.x = u; uv.y = v;
	return true;
}


Mesh::Mesh()
{
	objectType = ObjectType::Mesh;
}

Mesh::~Mesh()
{
	for (const Triangle* tri : allTris)
		delete tri;
}

bool Mesh::intersectObject(const Vec3f& orig, const Vec3f& dir, float& t0, Vec2f& uv) const
{
	std::cout << "Object intersect called with mesh\n";
	std::exit(-1);
}

bool Mesh::intersectMesh(const Vec3f& orig, const Vec3f& dir, float& t0, const Triangle*& triPtr, 
	Vec2f& uv) const
{
	return ac->intersectAccelStruct(orig, dir, t0, triPtr, uv);
}

void Mesh::getSurfaceData(const Vec3f& hitPoint, const Triangle* const triPtr, const Vec2f& uv, 
	Vec3f& hitNormal, Vec2f& tex) const
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
#ifdef _STATS
	stats::acCount.store(stats::acCount.load() + 1);
#endif // _STATS
}

AccelerationStructure::~AccelerationStructure(){}

void AccelerationStructure::setTris(std::vector<const Triangle*>& a_tris, int a_depth, const Options& options)
{
#ifdef _NO_ACCEL_STRUCT
	tris = a_tris;
#endif // _NO_ACCEL_STRUCT
	if (a_depth >= options.maxDepthAccelStruct) {
#ifdef _STATS
		stats::triCopiesCount.store(stats::triCopiesCount.load() + a_tris.size());
#endif // _STATS
		//std::cout << "depth " << a_depth << '\n';
		tris = a_tris;
		return;
	}

	using Pair = std::pair<std::vector<const Triangle*>*, AccelerationStructure*>;
	std::queue<Pair> queue;
	queue.push(Pair(&a_tris, this));

	std::vector<const Triangle*>* vec;
	AccelerationStructure* ac;

	while (!queue.empty()) {
		vec = queue.front().first;
		ac = queue.front().second;
		queue.pop();
		if (vec->size() < options.minBatchSizeAccelStruct || a_depth > options.maxDepthAccelStruct) {
#ifdef _STATS
			stats::triCopiesCount.store(stats::triCopiesCount.load() + vec->size());
#endif // _STATS
			//std::cout << "depth " << a_depth << '\n';
			ac->tris = *vec;
			continue;
		}

		int split; // xyz split
		Vec3f dim = bounds[1] - bounds[0];
		if (dim.x > dim.y && dim.x > dim.z)
			split = 0;
		else if (dim.y > dim.z)
			split = 1;
		else
			split = 2;
		float avg = 0.0f;
		std::vector<const Triangle*> trisLeft;
		std::vector<const Triangle*> trisRight;

		if (split == 0) {
			for (const Triangle* const tri : a_tris)
				avg += tri->a.x + tri->b.x + tri->c.x;
			avg /= 3.0f * a_tris.size();
			for (const Triangle* const tri : a_tris) {
				if (tri->a.x <= avg || tri->b.x <= avg || tri->c.x <= avg)
					trisLeft.push_back(tri);
				if (tri->a.x >= avg || tri->b.x >= avg || tri->c.x >= avg)
					trisRight.push_back(tri);
			}
		}
		else if (split == 1) {
			for (const Triangle* tri : a_tris)
				avg += tri->a.y + tri->b.y + tri->c.y;
			avg /= 3.0f * a_tris.size();
			for (const Triangle* tri : a_tris) {
				if (tri->a.y <= avg || tri->b.y <= avg || tri->c.y <= avg)
					trisLeft.push_back(tri);
				if (tri->a.y >= avg || tri->b.y >= avg || tri->c.y >= avg)
					trisRight.push_back(tri);
			}
		}
		else if (split == 2) {
			for (const Triangle* tri : a_tris)
				avg += tri->a.z + tri->b.z + tri->c.z;
			avg /= 3.0f * a_tris.size();
			for (const Triangle* tri : a_tris) {
				if (tri->a.z <= avg || tri->b.z <= avg || tri->c.z <= avg)
					trisLeft.push_back(tri);
				if (tri->a.z >= avg || tri->b.z >= avg || tri->c.z >= avg)
					trisRight.push_back(tri);
			}
		}
		else {
			assert(false);
		}

		if (split == 0) {
			left = std::make_unique<AccelerationStructure>();
			left->setBounds(bounds[0], Vec3f{ avg, bounds[1].y, bounds[1].z });
			left->setTris(trisLeft, a_depth + 1, options);
			right = std::make_unique<AccelerationStructure>();
			right->setBounds(Vec3f{ avg, bounds[0].y, bounds[0].z }, bounds[1]);
			right->setTris(trisRight, a_depth + 1, options);
		}
		else if (split == 1) {
			left = std::make_unique<AccelerationStructure>();
			left->setBounds(bounds[0], Vec3f{ bounds[1].x, avg, bounds[1].z });
			left->setTris(trisLeft, a_depth + 1, options);
			right = std::make_unique<AccelerationStructure>();
			right->setBounds(Vec3f{ bounds[0].x, avg, bounds[0].z }, bounds[1]);
			right->setTris(trisRight, a_depth + 1, options);
		}
		else {
			left = std::make_unique<AccelerationStructure>();
			left->setBounds(bounds[0], Vec3f{ bounds[1].x, bounds[1].y, avg });
			left->setTris(trisLeft, a_depth + 1, options);
			right = std::make_unique<AccelerationStructure>();
			right->setBounds(Vec3f{ bounds[0].x, bounds[0].y, avg }, bounds[1]);
			right->setTris(trisRight, a_depth + 1, options);
		}
	}
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
	stats::accelStructTests.store(stats::accelStructTests.load() + 1);
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

bool AccelerationStructure::intersectAccelStruct(const Vec3f& orig, const Vec3f& dir, float& t0, 
	const Triangle*& triPtr, Vec2f& uv) const
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

	for (const Triangle* tri : tris) {
		if (Triangle::rayTriangleIntersect(orig, dir, tri, tempT, tempUV) && tempT < t0) {
			inter = true;
			t0 = tempT;
			uv = tempUV;
			triPtr = tri;
		}
	}
	return inter;
}
