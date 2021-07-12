#include "timer.h"
#include "geometry.h"
#include "util.h"
#include "main.h"
#include "objects.h"
#include "lights.h"

#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <vector>
#include <map>

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
        // transparent objects do not cast shadows
        if (rayType == RayType::ShadowRay && (*i).get()->materialType == MaterialType::Transparent)
            continue;
        float tNear = std::numeric_limits<float>::max();
        int triIndex = -1;
        Vec2f uv;
        if ((*i).get()->intersect(orig, dir, tNear, triIndex, uv) && tNear < intrInfo.tNear) {
            intrInfo.hitObject = (*i).get();
            intrInfo.tNear = tNear;
            intrInfo.triIndex = triIndex;
            intrInfo.uv = uv;
        }
    }
    return (intrInfo.hitObject != nullptr);
}

std::map<int, int> triIndexMap;

Vec3f castRay(const Vec3f& orig, const Vec3f& dir,
    const ObjectVector& objects, const LightsVector& lights,
    const Options& options, const int depth)
{
    if (depth > options.maxDepth) return options.backgroundColor;
    IntersectInfo intrInfo;
    if (trace(orig, dir, objects, intrInfo)) {
        Vec3f hitColor = intrInfo.hitObject->color;
        Vec3f hitNormal;
        Vec2f hitTexCoordinates;
        Vec3f hitPoint = orig + dir * intrInfo.tNear;
        intrInfo.hitObject->getSurfaceData(hitPoint, intrInfo.triIndex, intrInfo.uv, hitNormal, hitTexCoordinates);

        if (intrInfo.triIndex >= 0) triIndexMap[intrInfo.triIndex]++;

        Vec3f objectColor = intrInfo.hitObject->color;
        // return hitNormal;
        //if (intrInfo.triIndex >= 0) objectColor = (Vec3f{ 1, 0, 0 } * hitTexCoordinates.x) + (Vec3f{ 0, 1, 0 } * hitTexCoordinates.y);
        //return objectColor;
        hitColor = { 0 };

        if (intrInfo.hitObject->materialType == MaterialType::Diffuse) {
            // for diffuse objects collect light from all visible sources
            for (size_t i = 0; i < lights.size(); ++i) {
                IntersectInfo intrShadInfo;
                Vec3f lightDir, lightIntensity;
                lights[i]->illuminate(hitPoint, lightDir, lightIntensity, intrShadInfo.tNear);
                bool vis = !trace(hitPoint + hitNormal * options.bias, -lightDir, objects, intrShadInfo, RayType::ShadowRay);
                float pattern = intrInfo.hitObject->getPattern(hitTexCoordinates);
                hitColor += (objectColor * lightIntensity) * pattern * vis * std::max(0.f, hitNormal.dotProduct(-lightDir));
                //hitColor += objectColor, intrShadInfo.tNear;
            }
        }
        else if (intrInfo.hitObject->materialType == MaterialType::Reflective) {
            // get info from reflected ray
            Vec3f reflectedRay = dir - 2 * dir.dotProduct(hitNormal) * hitNormal;
            hitColor = 0.95f * castRay(hitPoint + options.bias * hitNormal, reflectedRay, objects, lights, options, depth + 1);
        }
        else if (intrInfo.hitObject->materialType == MaterialType::Transparent) {
            float kr = fresnel(dir, hitNormal, intrInfo.hitObject->indexOfRefraction);
            bool outside = dir.dotProduct(hitNormal) < 0;
            Vec3f biasVec = options.bias * hitNormal;
            hitColor = { 0 };
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
        else if (intrInfo.hitObject->materialType == MaterialType::Phong) {
            Vec3f diffuseLight = 0, specularLight = 0;
            for (uint32_t i = 0; i < lights.size(); ++i) {
                Vec3f lightDir, lightIntensity;
                IntersectInfo isectShad;
                lights[i]->illuminate(hitPoint, lightDir, lightIntensity, isectShad.tNear);

                bool vis = !trace(hitPoint + hitNormal * options.bias, -lightDir, objects, isectShad, RayType::ShadowRay);
                // if (isectShad.tNear > lights[i])

                // compute the diffuse component
                diffuseLight += vis * intrInfo.hitObject->difuse * lightIntensity * std::max(0.f, hitNormal.dotProduct(-lightDir));

                // compute the specular component
                // what would be the ideal reflection direction for this light ray
                Vec3f R = reflect(lightDir, hitNormal);
                specularLight += vis * intrInfo.hitObject->specular * lightIntensity * std::pow(std::max(0.f, R.dotProduct(-dir)), intrInfo.hitObject->nSpecular); // * intrInfo.hitObject->kReflect)
                    // + (intrInfo.hitObject->color * (1 - intrInfo.hitObject->kReflect));
            }
            hitColor = objectColor * intrInfo.hitObject->ambient + diffuseLight + specularLight;
            hitColor = hitColor * intrInfo.hitObject->getPattern(hitTexCoordinates);
        }
        return hitColor;
    }
    return options.backgroundColor;
}


void renderWorker(const Options & options, const ObjectVector & objects, const LightsVector & lights,
    Vec3f * frameBuffer, size_t y0, size_t y1)
{
    const Vec3f orig = { 0, 0, 0 };
    const float scale = tanf(options.fov * 0.5f / 180.0f * M_PI);
    float imageAspectRatio = (options.width) / (float)options.height;

    for (size_t y = y0; y < y1; y++) {
        for (size_t x = 0; x < options.width; x++) {
            float xPix = (2 * (x + 0.5f) / (float)options.width - 1) * scale * imageAspectRatio;
            float yPix = -(2 * (y + 0.5f) / (float)options.height - 1) * scale;
            Vec3f dir = Vec3f(xPix, yPix, -1).normalize();
            frameBuffer[x + y * options.width] = castRay(orig, dir, objects, lights, options, 0);
        }
    }
}

void render(const Options & options, const ObjectVector & objects, const LightsVector & lights, Vec3f * frameBuffer)
{
    Timer t("Render scene");
#ifndef _DEBUG
    std::vector<std::thread*> threadPool;
    for (size_t i = 0; i < options.nWorkers; i++) {
        size_t y0 = options.height / options.nWorkers * i;
        size_t y1 = options.height / options.nWorkers * (i + 1);
        if (i + 1 == options.nWorkers) y1 = options.height;
        threadPool.push_back(new std::thread(renderWorker, std::cref(options), std::cref(objects), std::cref(lights), frameBuffer, y0, y1));
    }

    for (auto& t : threadPool)
        t->join();
#else
    renderWorker(options, objects, lights, frameBuffer, 0, options.height);
#endif

    //for (auto& p : triIndexMap) std::cout << p.first << ' ' << p.second << '\n';
}

bool loadScene(const std::string & sceneName, ObjectVector & objects, LightsVector & lights, Options & options)
{
    enum class BlockType { None, Options, Light, Object };
    std::map<std::string, BlockType> blockMap;
    blockMap["[options]"] = BlockType::Options;
    blockMap["[light]"] = BlockType::Light;
    blockMap["[object]"] = BlockType::Object;
    BlockType blockType = BlockType::None;

    std::string scenePath = options.imagePath + "\\" + sceneName;
    std::string str;
    std::ifstream ifs(scenePath, std::ifstream::in);
    std::getline(ifs, str);

    while (ifs.good()) {
        // pasrse everything here
        if (str.length() != 0) {
            if (str[0] != '#') {
                if (str[0] == '[') {
                    if (str.find("end") != std::string::npos) {
                        return true;
                    }
                    assert(blockMap.find(str) != blockMap.end());
                    blockType = blockMap[str];
                }
                else {
                    if (blockType == BlockType::Options) {
                        assert(str.find('=') != std::string::npos);
                        std::string_view strv(str.c_str(), str.find('='));
                        if (strv.find("width") != std::string::npos)
                            options.width = strToInt(str.substr(str.find('=') + 1));
                        else if (strv.find("height") != std::string::npos)
                            options.height = strToInt(str.substr(str.find('=') + 1));
                        else if (strv.find("fov") != std::string::npos)
                            options.fov = strToFloat(str.substr(str.find('=') + 1));
                        else if (strv.find("n_workers") != std::string::npos)
                            options.nWorkers = strToInt(str.substr(str.find('=') + 1));
                        else if (strv.find("image_name") != std::string::npos) {
                            std::string name = str.substr(str.find('=') + 1);
                            if (name[0] == '\"') name.erase(0, 1);
                            if (name[name.length() - 1] == '\"') name.erase(name.length() - 1, 1);
                            options.imageName = name;
                        }
                        else if (strv.find("max_depth") != std::string::npos)
                            options.maxDepth = strToInt(str.substr(str.find('=') + 1));
                    }
                    else if (blockType == BlockType::Light) {
                        std::vector<std::string> res;
                        std::stringstream lineStream(str);
                        std::string cell;
                        while (std::getline(lineStream, cell, ','))
                            res.push_back(cell);

                        if (res.size() < 9) {
                            std::cout << "Short light string\n";
                            break;
                        }

                        if (res[1].find("distant_light") != std::string::npos) {
                            Vec3f dir{ strToFloat(res[2]), strToFloat(res[3]), strToFloat(res[4]) };
                            Vec3f color{ strToFloat(res[5]), strToFloat(res[6]), strToFloat(res[7]) };
                            DistantLight* light = new DistantLight(dir, color, strToFloat(res[8]));
                            lights.emplace_back(std::unique_ptr<Light>(light));
                        }
                        else if (res[1].find("point_light") != std::string::npos) {
                            Vec3f pos{ strToFloat(res[2]), strToFloat(res[3]), strToFloat(res[4]) };
                            Vec3f color{ strToFloat(res[5]), strToFloat(res[6]), strToFloat(res[7]) };
                            PointLight* light = new PointLight(pos, color, strToFloat(res[8]));
                            lights.emplace_back(std::unique_ptr<Light>(light));
                        }
                        else {
                            std::cout << "Unknown light type\n";
                            break;
                        }
                    }
                    else if (blockType == BlockType::Object) {
                        std::vector<std::string> res;
                        std::stringstream lineStream(str);
                        std::string cell;
                        while (std::getline(lineStream, cell, ',')) {
                            cell.erase(std::remove(cell.begin(), cell.end(), '\t'), cell.end());
                            res.emplace_back(cell);
                        }
                        if (res.size() < 3) {
                            std::cout << "Short object string\n";
                            break;
                        }

                        Object* object;
                        size_t i = 0;
                        if (res[1].find("sphere") != std::string::npos) {
                            Vec3f pos{ strToFloat(res[2]), strToFloat(res[3]), strToFloat(res[4]) };
                            Vec3f color{ strToFloat(res[6]), strToFloat(res[7]), strToFloat(res[8]) };
                            object = new Sphere(pos, strToFloat(res[5]), color);
                            i = 9;
                        }
                        else if (res[1].find("plane") != std::string::npos) {
                            if (res.size() < 11) {
                                std::cout << "Short plane string\n";
                                break;
                            }
                            Vec3f pos{ strToFloat(res[2]), strToFloat(res[3]), strToFloat(res[4]) };
                            Vec3f normal{ strToFloat(res[5]), strToFloat(res[6]), strToFloat(res[7]) };
                            Vec3f color{ strToFloat(res[8]), strToFloat(res[9]), strToFloat(res[10]) };
                            object = new Plane(pos, normal, color);
                            i = 11;
                        }
                        else if (res[1].find("mesh") != std::string::npos) {
                            std::string path = res[2];
                            if (path[path.length() - 1] == '\"') path.erase(path.length() - 1, 1);
                            if (path[0] == '\"') path.erase(0, 1);
                            path = options.imagePath + "\\" + path;
                            Vec3f pos{ strToFloat(res[3]), strToFloat(res[4]), strToFloat(res[5]) };
                            Vec3f size{ strToFloat(res[6]), strToFloat(res[7]), strToFloat(res[8]) };
                            object = Mesh::loadOBJ(path, pos, size);
                        }
                        else {
                            std::cout << "Unknown object type\n";
                            break;
                        }

                        std::string pat = res[i];
                        if (pat.find("none") != std::string::npos)
                            object->pattern = PatternType::None;
                        else if (pat.find("stripped") != std::string::npos)
                            object->pattern = PatternType::Stripped;
                        else if (pat.find("shaded_chessboard") != std::string::npos)
                            object->pattern = PatternType::ShadedChessboard;
                        else if (pat.find("chessboard") != std::string::npos)
                            object->pattern = PatternType::Chessboard;

                        i++;

                        if (i < res.size()) {
                            if (res[i].find("reflective") != std::string::npos)
                                object->materialType = MaterialType::Reflective;
                            else if (res[i].find("transparent") != std::string::npos)
                                object->materialType = MaterialType::Transparent;
                            else if (res[i].find("phong") != std::string::npos) {
                                if (i + 4 >= res.size()) {
                                    std::cout << "Too short for phong\n";
                                }
                                else {
                                    object->materialType = MaterialType::Phong;
                                    object->ambient = strToFloat(res[i + 1]);
                                    object->difuse = strToFloat(res[i + 2]);
                                    object->specular = strToFloat(res[i + 3]);
                                    object->nSpecular = strToFloat(res[i + 4]);
                                }
                            }
                        }

                        objects.emplace_back(std::unique_ptr<Object>(object));
                    }
                }
            }
        }
        std::getline(ifs, str);
    }
    ifs.close();
    return false;
}

int renderScene(const std::string & sceneName) {
    Timer t("Total time");

    Options options;
    LightsVector lights;
    ObjectVector objects;

    if (!loadScene(sceneName, objects, lights, options))
        return -1;

    // std::cout << "Starting render " << options.width << "x" << options.height << std::endl;
    Vec3f* frameBuffer = new Vec3f[options.height * options.width];
    render(options, objects, lights, frameBuffer);
    savePPM(frameBuffer, options);
    delete[] frameBuffer;
    return 0;
}

int main()
{
    return renderScene("files\\scenes\\smooth_shading.scene");
    return 0;
}
