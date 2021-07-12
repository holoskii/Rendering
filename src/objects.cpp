#include "objects.h"
#include "util.h"

#include <iostream>
#include <sstream>
#include <fstream>

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
        res = (cos(texture.y * 2 * M_PI * scaleT * t) * sin(texture.x * 2 * M_PI * scaleS * s) + 1) * 0.5;
    return res < 0.1f ? 0.1f : res;
}


Sphere::Sphere(const Vec3f& a_center, const float a_r, const Vec3f& a_color,
    const MaterialType& a_materialType)
    : Object(a_center, a_color, a_materialType), r(a_r)
{
    r2 = r * r;
}

bool Sphere::intersect(const Vec3f& orig, const Vec3f& dir, float& t0, int& triIndex, Vec2f& uv) const
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

void Sphere::getSurfaceData(const Vec3f& hitPoint, const int triIndex, const Vec2f& uv, Vec3f& hitNormal, Vec2f& tex) const
{
    hitNormal = hitPoint - center;
    hitNormal.normalize();

    tex.x = (1 + atan2(hitNormal.z, hitNormal.x) / M_PI) * 0.5;
    tex.y = acosf(hitNormal.y) / M_PI;
}


Plane::Plane(const Vec3f& a_center, const Vec3f& a_normal, const Vec3f& a_color,
    const MaterialType& a_materialType)
    : Object(a_center, a_color, a_materialType), normal(a_normal)
{
    normal.normalize();
}

bool Plane::intersect(const Vec3f& orig, const Vec3f& dir, float& t0, int& triIndex, Vec2f& uv) const
{
    float denom = dir.dotProduct(normal);
    if (fabs(denom) < 1e-6)
        return false;
    t0 = ((center - orig).dotProduct(normal)) / denom;
    return (t0 >= 0);
}

void Plane::getSurfaceData(const Vec3f& hitPoint, const int triIndex, const Vec2f& uv, Vec3f& hitNormal, Vec2f& tex) const
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


Mesh::Mesh() {}

bool Mesh::rayTriangleIntersect(const Vec3f& orig, const Vec3f& dir,
    const Triangle& tri, float& t, Vec2f& uv) const
{
    const Vec3f& v0 = tri.a;
    const Vec3f& v1 = tri.b;
    const Vec3f& v2 = tri.c;
#if 0
    // compute plane's normal
    Vec3f v0v1 = v1 - v0;
    Vec3f v0v2 = v2 - v0;
    // no need to normalize
    Vec3f N = v0v1.crossProduct(v0v2); // N
    //Vec3f normal = (tri.n_a + tri.n_b + tri.n_c).normalize();
    // std::cout << N << " " << normal << '\n';
   
    
    float area2 = N.length();


    // Step 1: finding P

    // check if ray and plane are parallel ?
    float NdotRayDirection = N.dotProduct(dir);
    if (fabs(NdotRayDirection) < 1e-4) // almost 0 
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

    // if (-N.dotProduct(dir) < 0) return false;

    if (det < 0) return false;

    // ray and triangle are parallel if det is close to 0
    if (fabs(det) < 1e-4) return false;

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

bool Mesh::intersect(const Vec3f& orig, const Vec3f& dir, float& t0, int& triIndex, Vec2f& uv) const
{
    int index = 0;
    bool inter = false;
    t0 = std::numeric_limits<float>::max();
    for (const Triangle& tri : tris) {
        float t;
        if (rayTriangleIntersect(orig, dir, tri, t, uv) && t < t0) {
            inter = true;
            t0 = t;
            triIndex = index;
        }
        index++;
    }
    return inter;
}

void Mesh::getSurfaceData(const Vec3f& hitPoint, const int triIndex, const Vec2f& uv, Vec3f& hitNormal, Vec2f& tex) const
{
    const Vec3f& v0 = tris.at(triIndex).a;
    const Vec3f& v1 = tris.at(triIndex).b;
    const Vec3f& v2 = tris.at(triIndex).c;
    const Triangle& tri = tris.at(triIndex);
    hitNormal = ((tri.n_b * uv.x + tri.n_c * uv.y + tri.n_a * (1 - uv.x - uv.y)) / 3).normalize();
    //hitNormal = (v1 - v0).crossProduct(v2 - v0).normalize();
    tex = Vec2f{ uv.x, uv.y };
}

Mesh* Mesh::loadOBJ(const std::string& filename, const Vec3f& pos, const Vec3f& size)
{
    int count = 0;

    Mesh* mesh = new Mesh();

    std::cout << "Mesh: " << filename << '\n';
    std::vector<Vec3f> vertexData;
    std::vector<Vec3f> normalData;

    std::ifstream ifs(filename, std::ios::in);

    Vec3f min = { std::numeric_limits<float>::max() };
    Vec3f max = { std::numeric_limits<float>::min() };

    bool normalized = false;

    int dumpInt;
    char dumpChar;
    std::string line, dumpStr, type;
    do {
        std::getline(ifs, line);
        if (line.find('#') != std::string::npos)
            line.erase(line.find('#'));
        if (line.length() <= 0)
            continue;
        
        std::stringstream ss{ line };
        std::getline(ss, type, ' ');        
        
        if (type == std::string("v")) {
            
            assert(!normalized);
            float x, y, z;
            ss >> x >> y >> z;
            min.x = std::min(x, min.x);
            min.y = std::min(y, min.y);
            min.z = std::min(z, min.z);
            max.x = std::max(x, max.x);
            max.y = std::max(y, max.y);
            max.z = std::max(z, max.z);

            vertexData.emplace_back(Vec3f(x, y, z));
        } 
        else if (type == std::string("vn")) {
            float x, y, z;
            ss >> x >> y >> z;
            Vec3f normal = Vec3f{ x, y, z }.normalize();
            normalData.emplace_back(normal);
        }
        else if (type == std::string("f")) {
            count++;
            if (!normalized) {
                normalized = true;
                Vec3f normSize = size;
                normSize.y = normSize.x / ((max.x - min.x) / (max.y - min.y));
                normSize.z = normSize.x / ((max.x - min.x) / (max.z - min.z));
                for (auto& v: vertexData) {
                    v.x = normSize.x * ((v.x - min.x) / (max.x - min.x) - 0.5f) + pos.x;
                    v.y = normSize.y * ((v.y - min.y) / (max.y - min.y) - 0.5f) + pos.y;
                    v.z = normSize.z * ((v.z - min.z) / (max.z - min.z) - 0.5f) + pos.z;
                }
            }
            size_t v1, v2, v3, v4;
            size_t n1, n2, n3, n4;
            if (std::count(line.begin(), line.end(), '/') < 6) {
                ss >> v1 >> v2 >> v3;
                assert(v1 <= vertexData.size() && v2 <= vertexData.size() && v3 <= vertexData.size());
                mesh->tris.emplace_back(Triangle(vertexData.at(v1 - 1), vertexData.at(v2 - 1), vertexData.at(v3 - 1)));
            }
            else if (std::count(line.begin(), line.end(), '/') == 6) {
                ss >> v1 >> dumpChar >> dumpInt >> dumpChar >> n1;
                ss >> v2 >> dumpChar >> dumpInt >> dumpChar >> n2;
                ss >> v3 >> dumpChar >> dumpInt >> dumpChar >> n3;
                assert(v1 <= vertexData.size() && v2 <= vertexData.size() && v3 <= vertexData.size());
                assert(n1 <= normalData.size() && n2 <= normalData.size() && n3 <= normalData.size());
                mesh->tris.emplace_back(Triangle(vertexData.at(v1 - 1), vertexData.at(v2 - 1), vertexData.at(v3 - 1), normalData.at(n1 - 1), normalData.at(n2 - 1), normalData.at(n3 - 1)));
            }
            else if (std::count(line.begin(), line.end(), '/') == 8) {
                ss >> v1 >> dumpChar >> dumpInt >> dumpChar >> n1;
                ss >> v2 >> dumpChar >> dumpInt >> dumpChar >> n2;
                ss >> v3 >> dumpChar >> dumpInt >> dumpChar >> n3;
                ss >> v4 >> dumpChar >> dumpInt >> dumpChar >> n4;
                assert(v1 <= vertexData.size() && v2 <= vertexData.size() && v3 <= vertexData.size() && v4 <= vertexData.size());
                assert(n1 <= normalData.size() && n2 <= normalData.size() && n3 <= normalData.size() && n4 <= normalData.size());
                mesh->tris.emplace_back(Triangle(vertexData.at(v1 - 1), vertexData.at(v2 - 1), vertexData.at(v3 - 1), normalData.at(n1 - 1), normalData.at(n2 - 1), normalData.at(n3 - 1)));
                mesh->tris.emplace_back(Triangle(vertexData.at(v1 - 1), vertexData.at(v3 - 1), vertexData.at(v4 - 1), normalData.at(n1 - 1), normalData.at(n3 - 1), normalData.at(n4 - 1)));
            }
            else {
                std::cout << "unhandled polygon count\n";
            }
        }
    } while (ifs.good());
    ifs.close();
    return mesh;
}