#include "main.h"

enum class RayType { PrimaryRay, ShadowRay };
using ObjectVector = std::vector<std::unique_ptr<Object>>;
using LightsVector = std::vector<std::unique_ptr<Light>>;

struct IntersectInfo
{
    const Object* hitObject = nullptr;
    float tNear = std::numeric_limits<float>::max();
};

bool trace(const Vec3f& orig, const Vec3f& dir,
    const std::vector<std::unique_ptr<Object>>& objects,
    IntersectInfo& intrInfo, const RayType rayType = RayType::PrimaryRay)
{
    intrInfo.hitObject = nullptr;
    for (auto i = objects.begin(); i != objects.end(); i++) {
        float tNear = std::numeric_limits<float>::max();
        if ((*i).get()->intersect(orig, dir, tNear) && tNear < intrInfo.tNear) {
            intrInfo.hitObject = (*i).get();
            intrInfo.tNear = tNear;
        }
    }

    return (intrInfo.hitObject != nullptr);
}

Vec3f castRay(const Vec3f& orig, const Vec3f& dir,
    ObjectVector& objects, LightsVector& lights,
    const Options& options, const int depth)
{
    if (depth > options.maxDepth) return options.backgroundColor;
    IntersectInfo intrInfo;
    if (trace(orig, dir, objects, intrInfo)) {
        Vec3f hitColor;
        Vec3f hitNormal;
        Vec2f hitTexCoordinates;
        Vec3f hitPoint = orig + dir * intrInfo.tNear;
        intrInfo.hitObject->getSurfaceData(hitPoint, hitNormal, hitTexCoordinates);

        if (intrInfo.hitObject->materialType == MaterialType::Diffuse) {
            // for diffuse objects collect light from all visible sources
            for (size_t i = 0; i < lights.size(); ++i) {
                IntersectInfo isectShad;
                Vec3f lightDir, lightIntensity;
                lights[i]->illuminate(hitPoint, lightDir, lightIntensity, isectShad.tNear);
                bool vis = !trace(hitPoint + hitNormal * options.bias, -lightDir, objects, isectShad, RayType::ShadowRay);
                if (!vis && lights[i]->type == LightType::PointLight && isectShad.tNear >= (hitPoint - ((PointLight*)lights[i].get())->pos).length())
                    vis = true;
                if (vis) {
                    hitColor += (intrInfo.hitObject->color * lightIntensity) * vis * std::max(0.f, hitNormal.dotProduct(-lightDir));
                }
            }
        }
        else if (intrInfo.hitObject->materialType == MaterialType::Reflective) {
            // get info from reflected ray
            Vec3f reflectedRay = dir - 2 * dir.dotProduct(hitNormal) * hitNormal;
            hitColor = 0.95 * castRay(hitPoint + options.bias * hitNormal, reflectedRay, objects, lights, options, depth + 1);
        } 

        return hitColor;
    }
    return options.backgroundColor;
}

int savePPM(Vec3f* frameBuffer, Options& options)
{
    auto clamp = [](const float& lo, const float& hi, const float& v)
    {
        return std::max(lo, std::min(hi, v));
    };
    std::string path = options.imagePath + "\\" + options.imageName;
    std::ofstream of(path, std::ios::out | std::ios::binary);
    of << "P6\n" << std::to_string(options.width) << " "
        << std::to_string(options.height) << "\n" << std::to_string(255) << "\n";
    for (int i = 0; i < options.height * options.width; i++)
        for (int j = 0; j < 3; j++)
            of << static_cast<char>(clamp(0.05f, 1.0f, frameBuffer[i][j]) * 255);
    of.close();
    wchar_t* wPath = new wchar_t[strlen(path.c_str()) + 1];
    mbstowcs(wPath, path.c_str(), strlen(path.c_str()) + 1);
    ShellExecute(NULL, L"open", wPath, NULL, NULL, SW_SHOWDEFAULT);
    delete[] wPath;
    return 0;
}

void render(Options& options, ObjectVector& objects, LightsVector& lights)
{
    Vec3f* frameBuffer = new Vec3f[options.height * options.width];
    Vec3f orig = { 0, 0, 0 };
    float imageAspectRatio = (options.width) / (float)options.height;
    float scale = tan(options.fov * 0.5 / 180.0 * M_PI);
    for (int x = 0; x < options.width; x++) {
        for (int y = 0; y < options.height; y++) {
            float xPix = (2 * (x + 0.5) / (float)options.width - 1) * scale * imageAspectRatio;
            float yPix = -(2 * (y + 0.5) / (float)options.height - 1) * scale;
            Vec3f dir = Vec3f(xPix, yPix, -1).normalize();
            frameBuffer[x + y * options.width] = castRay(orig, dir, objects, lights, options, 0);
        }
    }
    savePPM(frameBuffer, options);
    delete[] frameBuffer;
}

int main()
{
    Options options;
    options.width = 800;
    options.height = 600;
    // options.width = 3840; options.height = 2160;
    // options.width = 1920; options.height = 1080;
    options.fov = 60;

    LightsVector lights;
    lights.push_back(std::unique_ptr<Light>(new DistantLight({ 0, -1, 0 },      { 1, 1, 1 },    0.2)));
    lights.push_back(std::unique_ptr<Light>(new PointLight  ({ -1, 1, -1.5 },   { 1, 1, 0 },  0.5)));

    ObjectVector objects;
    objects.emplace_back(std::unique_ptr<Object>(new Plane(Vec3f{ 0.0, -1.5, 0.0 }, Vec3f{ 0, 1, 0 }, Vec3f{ 1, 1, 1 })));

    objects.emplace_back(std::unique_ptr<Object>(new Sphere(Vec3f{ -2.0, 0.0, -4.0 }, 1.0, Vec3f{ 1, 0, 0 })));
    objects.emplace_back(std::unique_ptr<Object>(new Sphere(Vec3f{ -0.5, 2.0, -6.0 }, 0.5, Vec3f{ 0, 1, 0 })));
    objects.emplace_back(std::unique_ptr<Object>(new Sphere(Vec3f{ 2.0,  0.5, -4.0 }, 1.5, Vec3f{ 1, 1, 1 }, MaterialType::Reflective)));
    objects.emplace_back(std::unique_ptr<Object>(new Sphere(Vec3f{ -0.5, 0.5, -2.0 }, 0.3, Vec3f{ 1, 1, 1 })));
    
    render(options, objects, lights);
}

