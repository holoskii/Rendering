#include "main.h"

enum class RayType { PrimaryRay, ShadowRay };
using ObjectVector = std::vector<std::unique_ptr<Object>>;
using LightsVector = std::vector<std::unique_ptr<Light>>;

struct IntersectInfo
{
    const Object* hitObject = nullptr;
    float tNear = std::numeric_limits<float>::max();
};

inline float clamp(const float low, const float high, const float val)
{
    return std::max(low, std::min(high, val));
}

Vec3f reflect(const Vec3f& dir, const Vec3f& normal)
{
    return dir - 2 * dir.dotProduct(normal) * normal;
}

Vec3f refract(const Vec3f& dir, const Vec3f& normal, const float& indexOfRefraction)
{
    float n1 = 1, n2 = indexOfRefraction;   // n1 - air, n2 - object
    float cosi = clamp(-1, 1, dir.dotProduct(normal));
    Vec3f modNormal = normal;
    if (cosi < 0) {
        // outside the medium
        cosi = -cosi;
    }
    else {
        // inside the medium
        std::swap(n1, n2);
        modNormal = -normal;
    }
    float rri = n1 / n2; // relative refraction index
    float k = 1 - rri * rri * (1 - cosi * cosi); // critial angle test
    if (k < 0) 
        return 0;
    return rri * dir + (rri * cosi - sqrtf(k)) * modNormal;
}

float fresnel(const Vec3f& dir, const Vec3f& normal, const float& indexOfRefraction)
{
    float n1 = 1, n2 = indexOfRefraction;   // n1 - air, n2 - object
    float cosi = clamp(-1, 1, dir.dotProduct(normal));
    if (cosi > 0) {
        // outside the medium
        std::swap(n1, n2);
    }
    float sint = n1 / n2 * sqrtf(std::max(0.f, 1 - cosi * cosi));  // Snell's law
    
    float kr;
    if (sint >= 1) {
        // total internal reflection
        kr = 1;
    }
    else {
        // fresnell equation
        float cost = sqrtf(std::max(0.f, 1 - sint * sint));
        cosi = fabsf(cosi);
        float rs = ((n2 * cosi) - (n1 * cost)) / ((n2 * cosi) + (n1 * cost));
        float rp = ((n1 * cosi) - (n2 * cost)) / ((n1 * cosi) + (n2 * cost));
        kr = (rs * rs + rp * rp) / 2;
    }
    return kr;
}

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
    const ObjectVector& objects, const LightsVector& lights,
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

        if      (intrInfo.hitObject->materialType == MaterialType::Diffuse) {
            // for diffuse objects collect light from all visible sources
            for (size_t i = 0; i < lights.size(); ++i) {
                IntersectInfo intrShadInfo;
                Vec3f lightDir, lightIntensity;
                lights[i]->illuminate(hitPoint, lightDir, lightIntensity, intrShadInfo.tNear);
                bool vis = !trace(hitPoint + hitNormal * options.bias, -lightDir, objects, intrShadInfo, RayType::ShadowRay);
                if (!vis && lights[i]->type == LightType::PointLight && intrShadInfo.tNear >= (hitPoint - ((PointLight*)lights[i].get())->pos).length())
                    vis = true;
                if (vis) {
                    float pattern = options.getPattern(hitTexCoordinates);
                    hitColor += (intrInfo.hitObject->color * lightIntensity) * pattern * vis * std::max(0.f, hitNormal.dotProduct(-lightDir));
                }
            }
        }
        else if (intrInfo.hitObject->materialType == MaterialType::Reflective) {
            // get info from reflected ray
            Vec3f reflectedRay = dir - 2 * dir.dotProduct(hitNormal) * hitNormal;
            hitColor = 0.95 * castRay(hitPoint + options.bias * hitNormal, reflectedRay, objects, lights, options, depth + 1);
        }
        else if (intrInfo.hitObject->materialType == MaterialType::Transparent) {
            float kr = fresnel(dir, hitNormal, intrInfo.hitObject->indexOfRefraction);
            bool outside = dir.dotProduct(hitNormal) < 0;
            Vec3f biasVec = options.bias * hitNormal;
   
            if (kr < 1) {
                // compute refraction if it is not a case of total internal reflection
                Vec3f refractionDirection = refract(dir, hitNormal, intrInfo.hitObject->indexOfRefraction).normalize();
                Vec3f refractionRayOrig = outside ? hitPoint - biasVec : hitPoint + biasVec; // add bias
                Vec3f refractionColor = castRay(refractionRayOrig, refractionDirection, objects, lights, options, depth + 1);
                hitColor += refractionColor * (1 - kr);
            }

            Vec3f reflectionDirection = reflect(dir, hitNormal).normalize();
            Vec3f reflectionRayOrig = outside ? hitPoint + biasVec : hitPoint - biasVec;    // add bias
            Vec3f reflectionColor = castRay(reflectionRayOrig, reflectionDirection, objects, lights, options, depth + 1);
            hitColor += reflectionColor * kr;
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
    //options.width = 3840; options.height = 2160;
    options.width = 1920; options.height = 1080;
    options.fov = 60;

    LightsVector lights;
    lights.push_back(std::unique_ptr<Light>(new DistantLight({ 0, -1, 0 },      { 1, 1, 1 },    0.2)));
    lights.push_back(std::unique_ptr<Light>(new PointLight  ({ -1, 1, -1.5 },   { 1, 1, 0 },    0.5)));

    ObjectVector objects;
    objects.emplace_back(std::unique_ptr<Object>(new Plane (Vec3f{ 0.0, -1.5, 0.0 }, Vec3f{ 0, 1, 0 }, Vec3f{ 1, 1, 1 })));

    objects.emplace_back(std::unique_ptr<Object>(new Sphere(Vec3f{ -2.0, 0.0, -4.0 }, 1.0, Vec3f{ 1, 0, 0 })));
    objects.emplace_back(std::unique_ptr<Object>(new Sphere(Vec3f{ -0.5, 2.0, -6.0 }, 0.5, Vec3f{ 0, 1, 0 })));
    objects.emplace_back(std::unique_ptr<Object>(new Sphere(Vec3f{ 2.0,  0.5, -4.0 }, 1.5, Vec3f{ 1, 1, 1 }, MaterialType::Reflective)));
    objects.emplace_back(std::unique_ptr<Object>(new Sphere(Vec3f{ -0.5, 0.5, -2.0 }, 0.3, Vec3f{ 1, 1, 1 }, MaterialType::Transparent)));
    
    render(options, objects, lights);
}

