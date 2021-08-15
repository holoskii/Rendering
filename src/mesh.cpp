// classes describing polygon mesh, single polygon and acceleration structure
#include "mesh.h"

#include <cassert>
#include <tuple>
#include <queue>
#include <functional>
#include <fstream>

#include "stats.h"
#include "options.h"
#include "timer.h"

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

bool Triangle::rayTriangleIntersect(const Ray& ray, const Triangle* triPtr,
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
	Vec3f pvec = ray.dir.crossProduct(v0v2);
	float det = v0v1.dotProduct(pvec);

#ifdef _BACKFACE_CULLING
	if (det < 1e-8) return false;
#endif // _BACKFACE_CULLING

	if (fabs(det) < 1e-8) return false;

	float invDet = 1 / det;
	Vec3f tvec = ray.orig - v0;
	u = tvec.dotProduct(pvec) * invDet;
	if (u < 0 || u > 1) return false;

	Vec3f qvec = tvec.crossProduct(v0v1);
	v = ray.dir.dotProduct(qvec) * invDet;
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

bool Mesh::intersectMesh(const Ray& ray, float& t0, const Triangle*& triPtr,
	Vec2f& uv) const
{
	return ac->intersectAccelStruct(ray, t0, triPtr, uv);
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

bool Mesh::loadOBJ(const std::string& filename, const Options& options)
{
	const float& x = degToRad(rot.x);
	Matrix44f mx(
		1, 0, 0, 0,
		0, cosf(x), -sinf(x), 0,
		0, sinf(x), cosf(x), 0,
		0, 0, 0, 1
	);

	const float& y = degToRad(rot.y);
	Matrix44f my(
		cosf(y), 0, sinf(y), 0,
		0, 1, 0, 0,
		-sinf(y), 0, cosf(y), 0,
		0, 0, 0, 1
	);

	const float& z = degToRad(rot.z);
	Matrix44f mz(
		cosf(z), -sinf(z), 0, 0,
		sinf(z), cosf(z), 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1
	);

	Matrix44f rMatrix = mz * my * mx;

	// fast unsigned int read
	auto getUInt = [](const char*& ptr)
	{
		size_t val = 0;
		while (*ptr == ' ') ptr++;
		if (*ptr == '/') ptr++;
		while (*ptr && *ptr != ' ' && *ptr != '/')
			val = val * 10 + *ptr++ - '0';
		return val;
};

	Timer t("OBJ loading");
	std::ifstream ifs(filename, std::ios::in);
	if (!ifs.good()) return false;

	ac = std::make_unique<AccelerationStructure>();
	std::string line;
	bool normalized = false;
	std::vector<Vec3f> vertexData;
	std::vector<Vec3f> normalData;
	std::vector<const Triangle*> tris;
	Vec3f min = { std::numeric_limits<float>::max() };
	Vec3f max = { std::numeric_limits<float>::min() };

#ifndef _NO_OUTPUT
	std::cout << "Mesh: " << filename << '\n';
#endif // _NO_OUTPUT

	do {
		// read line and drop commented part
		std::getline(ifs, line);

		if (line.find('#') != std::string::npos) line.erase(line.find('#'));
		if (line.length() <= 0) continue;

		// separate line header
		const char* c_line = line.c_str();
		char lineHeader[32] = { 0 };
		int res = sscanf_s(c_line, "%s", lineHeader, 32u);
		if (res == 0) {
			return false;
		}
		c_line += strlen(lineHeader) + 1;

		if (strcmp(lineHeader, "v") == 0) {
			// read vertex
			float x, y, z;
			int res = sscanf(c_line, "%f %f %f", &x, &y, &z);
			assert(res == 3);
			min.x = std::min(x, min.x); min.y = std::min(y, min.y);
			min.z = std::min(z, min.z); max.x = std::max(x, max.x);
			max.y = std::max(y, max.y); max.z = std::max(z, max.z);
			vertexData.emplace_back(Vec3f(x, y, z));
		}
		else if (strcmp(lineHeader, "vn") == 0) {
			// read normal
			float x, y, z;
			int res = sscanf_s(c_line, "%f %f %f", &x, &y, &z);
			assert(res == 3);
			normalData.emplace_back(Vec3f{ x, y, z }.normalize());
		}
		else if (strcmp(lineHeader, "f") == 0) {
			// read face
			if (!normalized) {
				// normalize all vertices
				normalized = true;
				Vec3f range = max - min;
				Vec3f stretch = size / range;
				float maxStretch = std::max(stretch.x, std::max(stretch.y, stretch.z));

				Vec3f normSize = size;
				if (maxStretch == stretch.x) {
					normSize.y = normSize.x / (range.x / range.y);
					normSize.z = normSize.x / (range.x / range.z);
				}
				else if (maxStretch == stretch.y) {
					normSize.x = normSize.y / (range.y / range.x);
					normSize.z = normSize.y / (range.y / range.z);
				}
				else {
					normSize.x = normSize.z / (range.z / range.x);
					normSize.y = normSize.z / (range.z / range.y);
				}

				for (auto& v : vertexData) {
					v.x = normSize.x * ((v.x - min.x) / range.x - 0.5f);
					v.y = normSize.y * ((v.y - min.y) / range.y - 0.5f);
					v.z = normSize.z * ((v.z - min.z) / range.z - 0.5f);

					v = rMatrix.multVecMatrix(v);

					v.x += pos.x;
					v.y += pos.y;
					v.z += pos.z;
				}

				for (auto& n : normalData) {
					n = rMatrix.multVecMatrix(n);
				}

				ac->setBounds(pos - normSize / 2, pos + normSize / 2);
			}

			// add face
			int slashCount = 0;
			const char* ptr = c_line;
			while (*ptr) if (*ptr++ == '/') slashCount++;
			ptr = c_line;

			if (slashCount == 0) {
				std::vector<size_t> vi;
				size_t v = 1;
				while ((v = getUInt(ptr)) > 0)
					vi.push_back(v);
				for (size_t i = 1; i < vi.size() - 1; i++)
					tris.push_back(new Triangle(vertexData.at(vi.at(0) - 1), vertexData.at(vi.at(i) - 1), vertexData.at(vi.at(i + 1) - 1)));
			}
			else if (slashCount % 2 == 0) {
				std::vector<size_t> vi, ti, ni;
				size_t v = 1, t = 1, n = 1;
				while ((v = getUInt(ptr)) > 0) {
					t = getUInt(ptr);
					n = getUInt(ptr);
					vi.push_back(v);
					if (t > 0) ti.push_back(t);
					if (n > 0) ni.push_back(n);
				}
				if (ni.size() == 0) {
					for (size_t i = 1; i < vi.size() - 1; i++)
						tris.push_back(new Triangle(vertexData.at(vi.at(0) - 1), vertexData.at(vi.at(i) - 1), vertexData.at(vi.at(i + 1) - 1)));
				}
				else {
					assert(ni.size() == vi.size());
					for (size_t i = 1; i < vi.size() - 1; i++)
						tris.push_back(new Triangle(vertexData.at(vi.at(0) - 1), vertexData.at(vi.at(i) - 1), vertexData.at(vi.at(i + 1) - 1),
							normalData.at(ni.at(0) - 1), normalData.at(ni.at(i) - 1), normalData.at(ni.at(i + 1) - 1)));
				}
			}
			else {
				std::cout << "unhandled slash count: " << slashCount << '\n';
			}
		}
	} while (ifs.good());
	ifs.close();

	allTris.reserve(tris.size());
	for (const Triangle* tri : tris)
		allTris.push_back(tri);
	ac->setup(tris, 1, options);
#ifdef _STATS
	stats::meshCount.store(stats::meshCount.load() + allTris.size());
#endif // _STATS
	return true;
}


AccelerationStructure::AccelerationStructure()
{
#ifdef _STATS
	stats::acCount.store(stats::acCount.load() + 1);
#endif // _STATS
}

AccelerationStructure::~AccelerationStructure(){}

void AccelerationStructure::setup(std::vector<const Triangle*>& a_tris, int a_depth, const Options& options)
{
#ifdef _NO_ACCEL_STRUCT
	tris = a_tris;
#endif // _NO_ACCEL_STRUCT

	if (a_tris.size() < options.minBatchSizeAccelStruct || a_depth >= options.maxDepthAccelStruct) {
#ifdef _STATS
		stats::triCopiesCount.store(stats::triCopiesCount.load() + a_tris.size());
#endif // _STATS
		tris = a_tris;
		return;
	}

	int orientation; // xyz orientation
	Vec3f dim = bounds[1] - bounds[0];
	if (dim.x > dim.y && dim.x > dim.z) orientation = 0;
	else if (dim.y > dim.z) orientation = 1;
	else orientation = 2;
	std::vector<const Triangle*> trisLeft;
	std::vector<const Triangle*> trisRight;

	float splitDist = getOptimalSplit(a_tris, orientation, bounds, trisLeft, trisRight);

	if ((trisLeft.size() == 0 || trisRight.size() == 0) || (trisLeft.size() + trisRight.size() >= a_tris.size() * 1.5)) {
#ifdef _STATS
		stats::triCopiesCount.store(stats::triCopiesCount.load() + a_tris.size());
#endif // _STATS
		tris = a_tris;
		return;
	}

	left = std::make_unique<AccelerationStructure>();
	right = std::make_unique<AccelerationStructure>();

	if (orientation == 0) {
		left->setBounds(bounds[0], Vec3f{ splitDist, bounds[1].y, bounds[1].z });
		right->setBounds(Vec3f{ splitDist, bounds[0].y, bounds[0].z }, bounds[1]);
	}
	else if (orientation == 1) {
		left->setBounds(bounds[0], Vec3f{ bounds[1].x, splitDist, bounds[1].z });
		right->setBounds(Vec3f{ bounds[0].x, splitDist, bounds[0].z }, bounds[1]);
	}
	else {
		left->setBounds(bounds[0], Vec3f{ bounds[1].x, bounds[1].y, splitDist });
		right->setBounds(Vec3f{ bounds[0].x, bounds[0].y, splitDist }, bounds[1]);
	}

	right->setup(trisRight, a_depth + 1, options);
	left->setup(trisLeft, a_depth + 1, options);

}

void AccelerationStructure::setBounds(const Vec3f& a, const Vec3f& b)
{
	bounds[0] = a;
	bounds[1] = b;
}

bool AccelerationStructure::intersectBox(const Ray& ray) const
{
#ifdef _NO_ACCEL_STRUCT
	return true;
#endif // _NO_ACCEL_STRUCT
#ifdef _STATS
	stats::accelStructTests.store(stats::accelStructTests.load() + 1);
#endif // _STATS
	const Vec3f invdir = 1 / ray.dir;
	const int sign[3] = { (invdir.x < 0), (invdir.y < 0), (invdir.z < 0) };

	float tmin, tmax, tymin, tymax, tzmin, tzmax;
	tmin = (bounds[sign[0]].x - ray.orig.x) * invdir.x;
	tmax = (bounds[1 - sign[0]].x - ray.orig.x) * invdir.x;
	tymin = (bounds[sign[1]].y - ray.orig.y) * invdir.y;
	tymax = (bounds[1 - sign[1]].y - ray.orig.y) * invdir.y;

	if ((tmin > tymax) || (tymin > tmax))
		return false;
	if (tymin > tmin)
		tmin = tymin;
	if (tymax < tmax)
		tmax = tymax;

	tzmin = (bounds[sign[2]].z - ray.orig.z) * invdir.z;
	tzmax = (bounds[1 - sign[2]].z - ray.orig.z) * invdir.z;

	if ((tmin > tzmax) || (tzmin > tmax))
		return false;
	if (tzmin > tmin)
		tmin = tzmin;
	if (tzmax < tmax)
		tmax = tzmax;

	return true;
}

int AccelerationStructure::recCountAC(const Ray& ray)
{
	if (intersectBox(ray)) {
		int result = 1;
		if (left.get() != nullptr)
			result += left.get()->recCountAC(ray);
		if (right.get() != nullptr)
			result += right.get()->recCountAC(ray);
		return result;
	}
	else {
		return 0;
	}
}

bool AccelerationStructure::intersectAccelStruct(const Ray& ray, float& t0,
	const Triangle*& triPtr, Vec2f& uv) const
{
	if (!this->intersectBox(ray))
		return false;

	bool inter = false;
	float tempT;
	Vec2f tempUV;
	const Triangle* tempTriPtr;
	t0 = std::numeric_limits<float>::max();
	if (left) {
		assert(right);
		if (left->intersectAccelStruct(ray, tempT, tempTriPtr, tempUV) && tempT < t0) {
			inter = true;
			t0 = tempT;
			uv = tempUV;
			triPtr = tempTriPtr;
		}
		if (right->intersectAccelStruct(ray, tempT, tempTriPtr, tempUV) && tempT < t0) {
			inter = true;
			t0 = tempT;
			uv = tempUV;
			triPtr = tempTriPtr;
		}


		return inter;
	}

	for (const Triangle* tri : tris) {
		if (Triangle::rayTriangleIntersect(ray, tri, tempT, tempUV) && tempT < t0) {
			inter = true;
			t0 = tempT;
			uv = tempUV;
			triPtr = tri;
		}
	}
	return inter;
}

float AccelerationStructure::calculateSAH(const int orientation, const std::vector<const Triangle*>& tris, 
	const Vec3f bounds[2], const float boundary)
{
	float sah = 0.0f;
	int triLeft = 0;
	int triRight = 0;
	if (orientation == 0) {
		assert(bounds[0].x <= boundary && bounds[1].x >= boundary);
		for (const Triangle* tri : tris) {
			if (tri->a.x <= boundary || tri->b.x <= boundary || tri->c.x <= boundary)
				triLeft++;
			if (tri->a.x >= boundary || tri->b.x >= boundary || tri->c.x >= boundary)
				triRight++;
		}
		sah = triLeft * (boundary - bounds[0].x) + triRight * (bounds[1].x - boundary);
	}
	else if (orientation == 1) {
		assert(bounds[0].y <= boundary && bounds[1].y >= boundary);
		for (const Triangle* tri : tris) {
			if (tri->a.y <= boundary || tri->b.y <= boundary || tri->c.y <= boundary)
				triLeft++;
			if (tri->a.y >= boundary || tri->b.y >= boundary || tri->c.y >= boundary)
				triRight++;
		}
		sah = triLeft * (boundary - bounds[0].y) + triRight * (bounds[1].y - boundary);
	}
	else if (orientation == 2) {
		assert(bounds[0].z <= boundary && bounds[1].z >= boundary);
		for (const Triangle* tri : tris) {
			if (tri->a.z <= boundary || tri->b.z <= boundary || tri->c.z <= boundary)
				triLeft++;
			if (tri->a.z >= boundary || tri->b.z >= boundary || tri->c.z >= boundary)
				triRight++;
		}
		sah = triLeft * (boundary - bounds[0].z) + triRight * (bounds[1].z - boundary);
	}
	return sah;
}

float AccelerationStructure::binarySearchSAH(const int orientation, const std::vector<const Triangle*>& tris, 
	const Vec3f bounds[2], const float left, const float right)
{
	float mid = right - (right - left) / 2;
	if (right - left < 0.1f) return mid;
	if (calculateSAH(orientation, tris, bounds, mid - 0.05f)
		< calculateSAH(orientation, tris, bounds, mid + 0.05f)) {
		return binarySearchSAH(orientation, tris, bounds, left, mid);
	}
	else {
		return binarySearchSAH(orientation, tris, bounds, mid, right);
	}
}

float AccelerationStructure::getOptimalSplit(const std::vector<const Triangle*>& tris, const int orientation, 
	const Vec3f bounds[2], std::vector<const Triangle*>& trisLeft, std::vector<const Triangle*>& trisRight)
{
	float splitDist = 0.0f;
#if 0
	// divide by equal parts
	if (orientation == 0) {
		splitDist = (bounds[0] + (bounds[1] - bounds[0]) / 2).x;
	}
	else if (orientation == 1) {
		splitDist = (bounds[0] + (bounds[1] - bounds[0]) / 2).y;
	}
	else if (orientation == 2) {
		splitDist = (bounds[0] + (bounds[1] - bounds[0]) / 2).z;
	}
#elif 1
	// Surface Area Heuristic (SAH)
	if (orientation == 0) {
		splitDist = binarySearchSAH(orientation, tris, bounds, bounds[0].x, bounds[1].x);
	}
	else if (orientation == 1) {
		splitDist = binarySearchSAH(orientation, tris, bounds, bounds[0].y, bounds[1].y);
	}
	else if (orientation == 2) {
		splitDist = binarySearchSAH(orientation, tris, bounds, bounds[0].z, bounds[1].z);
	}
#else
	// divide by average
	if (orientation == 0) {
		for (const Triangle* const tri : tris)
			splitDist += tri->a.x + tri->b.x + tri->c.x;
		splitDist /= 3.0f * tris.size();
	}
	else if (orientation == 1) {
		for (const Triangle* tri : tris)
			splitDist += tri->a.y + tri->b.y + tri->c.y;
		splitDist /= 3.0f * tris.size();
	}
	else if (orientation == 2) {
		for (const Triangle* tri : tris)
			splitDist += tri->a.z + tri->b.z + tri->c.z;
		splitDist /= 3.0f * tris.size();
}
#endif

	// split between left and right (some tris will be dublicated)
	if (orientation == 0) {
		for (const Triangle* const tri : tris) {
			if (tri->a.x <= splitDist || tri->b.x <= splitDist || tri->c.x <= splitDist)
				trisLeft.push_back(tri);
			if (tri->a.x >= splitDist || tri->b.x >= splitDist || tri->c.x >= splitDist)
				trisRight.push_back(tri);
		}
	}
	else if (orientation == 1) {
		for (const Triangle* tri : tris) {
			if (tri->a.y <= splitDist || tri->b.y <= splitDist || tri->c.y <= splitDist)
				trisLeft.push_back(tri);
			if (tri->a.y >= splitDist || tri->b.y >= splitDist || tri->c.y >= splitDist)
				trisRight.push_back(tri);
		}
	}
	else if (orientation == 2) {
		for (const Triangle* tri : tris) {
			if (tri->a.z <= splitDist || tri->b.z <= splitDist || tri->c.z <= splitDist)
				trisLeft.push_back(tri);
			if (tri->a.z >= splitDist || tri->b.z >= splitDist || tri->c.z >= splitDist)
				trisRight.push_back(tri);
		}
	}

	return splitDist;
}

int AccelerationStructure::countAC(const Ray& ray, const Scene& scene)
{
	int sum = 0;
	for (auto& obj : scene.objects) {
		if (obj->objectType == ObjectType::Mesh) {
			Mesh* mesh = dynamic_cast<Mesh*>(obj.get());
			sum += mesh->ac.get()->recCountAC(ray);
		}
	}
	return sum;
}

