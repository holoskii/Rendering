#include "objects.h"
#include "timer.h"
#include "util.h"
#include "renderer.h"

Mesh::Mesh() 
{
	objectType = ObjectType::Mesh;
}

bool Mesh::intersectObject(const Vec3f& orig, const Vec3f& dir, float& t0, Vec2f& uv) const
{
	std::cout << "Object intersect called with mesh\n";
	std::exit(-1);
}

bool Mesh::intersectMesh(const Vec3f& orig, const Vec3f& dir, float& t0, const Triangle*& triPtr, Vec2f& uv) const
{
	return accelStruct.intersectAccelStruct(orig, dir, t0, triPtr, uv);
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


AccelerationStructure::AccelerationStructure() {}

void AccelerationStructure::setTris(std::vector<Triangle> a_tris)
{
	tris = std::move(a_tris);
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
	if (!intersectBox(orig, dir)) return false;

	//const Triangle* tri = nullptr;
	bool inter = false;
	Vec2f tempuv;
	t0 = std::numeric_limits<float>::max();
	for (const Triangle& tri : tris) {
		float t;
		if (Triangle::rayTriangleIntersect(orig, dir, tri, t, tempuv) && t < t0) {
			inter = true;
			t0 = t;
			uv = tempuv;
			triPtr = &tri;
		}
	}
	return inter;
}

const Triangle& AccelerationStructure::getTri(size_t index) const
{
	return std::ref(tris.at(index));
}


Object::Object(const Vec3f& a_center, const Vec3f& a_color,
	const MaterialType& a_materialType)
	: color(a_color), center(a_center), materialType(a_materialType) {}

float Object::getPattern(Vec2f texture) const
{
	if (pattern == PatternType::None)
		return 1.0f;
	float scaleS = 10, scaleT = 10;
	float angle = degToRad(45);
	float s = texture.x * cos(angle) - texture.y * sin(angle);
	float t = texture.y * cos(angle) + texture.x * sin(angle);
	float res = 1.0f;

	if (pattern == PatternType::Stripped)
		res = (modulo(s * scaleS) < 0.5);
	else if (pattern == PatternType::Chessboard)
		res = (float)((modulo(s * scaleS) < 0.5) ^ (modulo(t * scaleT) < 0.5));
	else if (pattern == PatternType::ShadedChessboard)
		res = (cos(texture.y * 2 * M_PI * scaleT * t) * sin(texture.x * 2 * M_PI * scaleS * s) + 1) * 0.5f;
	return res < 0.1f ? 0.1f : res;
}


Sphere::Sphere(const Vec3f& a_center, const float a_r, const Vec3f& a_color,
	const MaterialType& a_materialType)
	: Object(a_center, a_color, a_materialType), r(a_r)
{
	r2 = r * r;
	objectType = ObjectType::Sphere;
}

bool Sphere::intersectObject(const Vec3f& orig, const Vec3f& dir, float& t0, Vec2f& uv) const
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

void Sphere::getSurfaceData(const Vec3f& hitPoint, const Triangle* const triPtr, const Vec2f& uv, Vec3f& hitNormal, Vec2f& tex) const
{
	hitNormal = hitPoint - center;
	hitNormal.normalize();

	tex.x = (1.0f + atan2(hitNormal.z, hitNormal.x) / M_PI) * 0.5f;
	tex.y = acosf(hitNormal.y) / M_PI;
}


Plane::Plane(const Vec3f& a_center, const Vec3f& a_normal, const Vec3f& a_color,
	const MaterialType& a_materialType)
	: Object(a_center, a_color, a_materialType), normal(a_normal)
{
	normal.normalize();
	objectType = ObjectType::Plane;
}

bool Plane::intersectObject(const Vec3f& orig, const Vec3f& dir, float& t0, Vec2f& uv) const
{
	float denom = dir.dotProduct(normal);
	if (fabs(denom) < 1e-8)
		return false;
	t0 = ((center - orig).dotProduct(normal)) / denom;
	return (t0 >= 0);
}

void Plane::getSurfaceData(const Vec3f& hitPoint, const Triangle* const triPtr, const Vec2f& uv, Vec3f& hitNormal, Vec2f& tex) const
{
	hitNormal = normal;

	Vec3f dist = hitPoint - center;
	tex.x = dist.x / 15;
	tex.y = dist.z / 15;
}


Triangle::Triangle(const Vec3f& a_a, const Vec3f& a_b, const Vec3f& a_c)
	: a(a_a), b(a_b), c(a_c)
{
	n_a = n_b = n_c = (a_b - a_a).crossProduct(a_c - a_a);
}

Triangle::Triangle(const Vec3f& a_a, const Vec3f& a_b, const Vec3f& a_c, const Vec3f& a_n_a, const Vec3f& a_n_b, const Vec3f& a_n_c)
	: Triangle(a_a, a_b, a_c)
{
	n_a = a_n_a;
	n_b = a_n_b;
	n_c = a_n_c;
}

bool Triangle::rayTriangleIntersect(const Vec3f& orig, const Vec3f& dir, const Triangle& tri,
	float& t, Vec2f& uv)
{
#ifdef _STATS
	rayTriTests.store(rayTriTests.load() + 1);
#endif // _STATS
	const Vec3f& v0 = tri.a;
	const Vec3f& v1 = tri.b;
	const Vec3f& v2 = tri.c;
#if 0
	// compute plane's normal
	Vec3f v0v1 = ;
	Vec3f v0v2 = ;
	// no need to normalize
	Vec3f N = (v1 - v0).crossProduct(v2 - v0); // N
	//Vec3f normal = (tri.n_a + tri.n_b + tri.n_c).normalize();
	// std::cout << N << " " << normal << '\n';


	float area2 = N.length();


	// Step 1: finding P

	// check if ray and plane are parallel ?
	float NdotRayDirection = N.dotProduct(dir);
	if (fabs(NdotRayDirection) < 1e-8) // almost 0 
		return false; // they are parallel so they don't intersect ! 

	// compute d parameter using equation 2
	float d = N.dotProduct(v0);

	// compute t (equation 3)
	t = (N.dotProduct(orig) + d) / NdotRayDirection;
	// check if the triangle is in behind the ray
	if (t < 0) return false; // the triangle is behind 

	// compute the intersection point using equation 1
	Vec3f P = orig + t * dir;

	// Step 2: inside-outside test
	Vec3f C; // vector perpendicular to triangle's plane 

	// edge 0
	Vec3f edge0 = v1 - v0;
	Vec3f vp0 = P - v0;
	C = edge0.crossProduct(vp0);
	if (N.dotProduct(C) < 0) return false; // P is on the right side 

	// edge 1
	Vec3f edge1 = v2 - v1;
	Vec3f vp1 = P - v1;
	C = edge1.crossProduct(vp1);
	if (N.dotProduct(C) < 0)  return false; // P is on the right side 

	// edge 2
	Vec3f edge2 = v0 - v2;
	Vec3f vp2 = P - v2;
	C = edge2.crossProduct(vp2);
	if (N.dotProduct(C) < 0) return false; // P is on the right side; 

	return true; // this ray hits the triangle 
#else
	float u, v;
	Vec3f v0v1 = v1 - v0;
	Vec3f v0v2 = v2 - v0;
	Vec3f pvec = dir.crossProduct(v0v2);
	float det = v0v1.dotProduct(pvec);

	// backface culling
	// if (det < 1e-8) return false;

	// ray and triangle are parallel if det is close to 0
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
#endif
}
