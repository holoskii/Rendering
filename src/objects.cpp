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
    : a(a_a), b(a_b), c(a_c) {};


Mesh::Mesh(const std::string& filename)
{
    parseOBJ(*this, filename);
}

bool Mesh::rayTriangleIntersect(const Vec3f& orig, const Vec3f& dir,
    const Triangle& tri, float& t, Vec2f& uv) const
{
    const Vec3f& v0 = tri.a;
    const Vec3f& v1 = tri.b;
    const Vec3f& v2 = tri.c;

    Vec3f v0v1 = v1 - v0;
    Vec3f v0v2 = v2 - v0;
    Vec3f pvec = dir.crossProduct(v0v2);
    float det = v0v1.dotProduct(pvec);

    if (det < 0) return false;

    // ray and triangle are parallel if det is close to 0
    if (fabs(det) < 1e-6) return false;

    float invDet = 1 / det;
    Vec3f tvec = orig - v0;

    float u = tvec.dotProduct(pvec) * invDet;
    if (u < 0 || u > 1) return false;

    Vec3f qvec = tvec.crossProduct(v0v1);
    float v = dir.dotProduct(qvec) * invDet;
    if (v < 0 || u + v > 1) return false;

    t = v0v2.dotProduct(qvec) * invDet;
    if (t < 0) return false;

    uv.x = u; uv.y = v;
    return true;
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
    hitNormal = (v1 - v0).crossProduct(v2 - v0).normalize();
    tex = Vec2f{ uv.x, uv.y };
}

void Mesh::parseOBJ(Mesh& mesh, const std::string& filename)
{
    std::cout << "Mesh path: " << filename << '\n';
    std::vector<Vec3f> vertexData;

    std::ifstream ifs(filename, std::ios::in);

    std::string line, dumpStr;
    do {
        std::getline(ifs, line);
        if (line[0] == '#') {
            continue;
        }
        else {
            std::stringstream ss{ line };
            if (line[0] == 'v') {
                float x, y, z;
                ss >> dumpStr >> x >> y >> z;
                z -= 5;
                vertexData.emplace_back(Vec3f(x, y, z));
            }
            else if (line[0] == 'f') {
                size_t x, y, z;
                if (std::count(line.begin(), line.end(), '/') >= 6) {
                    ss >> dumpStr >> x >> dumpStr;
                    ss >> y >> dumpStr;
                    ss >> z >> dumpStr;
                    mesh.tris.emplace_back(Triangle(vertexData.at(x - 1), vertexData.at(y - 1), vertexData.at(z - 1)));
                }
                else {
                    ss >> dumpStr >> x >> y >> z;
                    if (x > vertexData.size() || y > vertexData.size() || z > vertexData.size()) {
                        std::cout << "Wrong .obj indexing\n";
                        exit(-1);
                    }
                    mesh.tris.emplace_back(Triangle(vertexData.at(x - 1), vertexData.at(y - 1), vertexData.at(z - 1)));
                }

            }
        }

    } while (ifs.good());
}