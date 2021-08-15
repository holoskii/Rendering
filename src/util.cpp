// utility functions and functions working with file IO
#include "util.h"

#define NOMINMAX
#include "windows.h"
#include "shellapi.h"
#include <fstream>
#include <algorithm>
#include <map>
#include <string>

#include "options.h"
#include "stats.h"
#include "timer.h"
#include "renderer.h"

int savePPM(Vec3f* frameBuffer, const Options& options)
{
    // Timer t("Save image");

    std::string path = options.imagePath + "\\" + options.imageName;
    std::ofstream of(path, std::ios::out | std::ios::binary);
    if (!of.good()) {
        std::cout << "Error during file output\n";
        return -1;
    }

    // intermediate buffer significantly speeds up file write process
    char* data = new char[options.height * options.width * 3];
    char* dataPtr = data;
    for (int i = 0; i < options.height * options.width; i++)
        for (unsigned char j = 0; j < 3; j++)
            *dataPtr++ = static_cast<char>(clamp(0.0f, 1.0f, frameBuffer[i][j]) * 255);

    of << "P6\n" << std::to_string(options.width) << " " << std::to_string(options.height) << "\n" << std::to_string(255) << "\n";
    of.write(data, options.height * options.width * 3);
    of.close();

    wchar_t* wPath = new wchar_t[strlen(path.c_str()) + 1];
    mbstowcs(wPath, path.c_str(), strlen(path.c_str()) + 1);
    ShellExecute(NULL, L"open", wPath, NULL, NULL, SW_SHOWDEFAULT);
    delete[] wPath;
    delete[] data;
    return 0;
}

bool loadScene(Scene& scene, const std::string& sceneName)
{
#ifndef _NO_OUTPUT
    std::cout << "Loading scene " << sceneName << '\n';
#endif // _NO_OUTPUT

    enum class BlockType { None, Options, Light, Object };
    std::map<std::string, BlockType> blockMap;
    blockMap["[options]"] = BlockType::Options;
    blockMap["[light]"] = BlockType::Light;
    blockMap["[object]"] = BlockType::Object;
    blockMap["[end]"] = BlockType::None;
    BlockType blockType = BlockType::None;

    std::string scenePath = scene.options.imagePath + "\\" + sceneName;
    std::string str;
    std::ifstream ifs(scenePath, std::ifstream::in);

    Light* light = nullptr;
    Object* object = nullptr;

    while (ifs.good()) {
        std::getline(ifs, str);
        if (str.length() == 0)
            continue;

        // finish previous block
        if (strContains(str, "[")) {
            if (blockType == BlockType::Light) {
                assert(light != nullptr);
                scene.lights.push_back(std::unique_ptr<Light>(light));
            }
            else if (blockType == BlockType::Object) {
                assert(object != nullptr);
                scene.objects.push_back(std::unique_ptr<Object>(object));
            }
        }

        // skip commented block
        if (strContains(str, "#[")) {
            do {
                std::getline(ifs, str);
            } while (!strContains(str, "[") || strContains(str, "#["));
            if (!ifs.good())
                break;
        }

        // skip comments
        if (strContains(str, "#"))
            str.erase(str.find('#'));
        if (str.length() == 0)
            continue;

        // select block
        if (str[0] == '[') {
            assert(blockMap.find(str) != blockMap.end());
            blockType = blockMap[str];
            if (blockType == BlockType::None)
                break;
            continue;
        }
        
        // parse the line
        if (blockType == BlockType::Options) {
            assert(strContains(str, "="));
            std::string_view key(str.c_str(), str.find('='));
            std::string_view value(str.c_str() + str.find('=') + 1);

            if      (strEquals(key, "width"))
                scene.options.width = strToInt(value);
            else if (strEquals(key, "height"))
                scene.options.height = strToInt(value);
            else if (strEquals(key, "fov"))
                scene.options.fov = strToFloat(value);
            else if (strEquals(key, "image_name"))
                scene.options.imageName = std::string(value);
            else if (strEquals(key, "n_workers="))
                scene.options.nWorkers = strToInt(value);
            else if (strEquals(key, "max_ray_depth"))
                scene.options.maxRayDepth = strToInt(value);
            else if (strEquals(key, "ac_max_depth"))
                scene.options.maxDepthAccelStruct = strToInt(value);
            else if (strEquals(key, "ac_min_batch"))
                scene.options.minBatchSizeAccelStruct = strToInt(value);
            else if (strEquals(key, "background_color"))
                scene.options.backgroundColor = str3ToFloat(splitString(value, ','));
            else if (strEquals(key, "position"))
                scene.camera.pos = str3ToFloat(splitString(value, ','));
            else if (strEquals(key, "rotation"))
                scene.camera.rot = str3ToFloat(splitString(value, ','));
        }
        else if (blockType == BlockType::Light) {
            assert(strContains(str, "="));
            std::string_view key(str.c_str(), str.find('='));
            std::string_view value(str.c_str() + str.find('=') + 1);

            if (strEquals(key, "type")) {
                if (strEquals(value, "distant"))
                    light = new DistantLight();
                else if (strEquals(value, "point"))
                    light = new PointLight();
            }
            else if (light == nullptr)
                std::cout << "Error, light type missing\n";
            else if (strEquals(key, "color"))
                light->color = str3ToFloat(splitString(value, ','));
            else if (strEquals(key, "intensity"))
                light->intensity = strToFloat(value);
            if (strEquals(key, "direction")) {
                assert(light->type == LightType::DistantLight);
                static_cast<DistantLight*>(light)->dir = str3ToFloat(splitString(value, ','));
            }
            else if (strEquals(key, "position")) {
                assert(light->type == LightType::PointLight);
                static_cast<PointLight*>(light)->pos = str3ToFloat(splitString(value, ','));
            }
        }
        else if (blockType == BlockType::Object) {
            assert(strContains(str, "="));
            std::string_view key(str.c_str(), str.find('='));
            std::string_view value(str.c_str() + str.find('=') + 1);

            if (strEquals(key, "type")) {
                if (strEquals(value, "plane"))
                    object = new Plane();
                else if (strEquals(value, "sphere"))
                    object = new Sphere();
                else if (strEquals(value, "mesh"))
                    object = new Mesh();
            }
            else if (object == nullptr) {
                std::cout << "Error, object type missing\n";
            }
            else if (strEquals(key, "color")) {
                object->color = str3ToFloat(splitString(value, ','));
            }
            else if (strEquals(key, "pos")) {
                object->pos = str3ToFloat(splitString(value, ','));
            }
            else if (strEquals(key, "pattern")) {
                if (strEquals(value, "chessboard")) {
                    object->pattern = PatternType::Chessboard;
                }
            }
            else if (strEquals(key, "material")) {
                auto res = splitString(value, ',');
                if (strEquals(res[0], "transparent")) {
                    object->materialType = MaterialType::Transparent;
                }
                else if (strEquals(res[0], "reflective")) {
                    object->materialType = MaterialType::Reflective;
                }
                if (strEquals(res[0], "phong")) {
                    object->materialType = MaterialType::Phong;
                    object->ambient = strToFloat(res[1]);
                    object->difuse = strToFloat(res[2]);
                    object->specular = strToFloat(res[3]);
                    object->nSpecular = strToFloat(res[4]);
                }
            }
            else if (object->objectType == ObjectType::Sphere) {
                Sphere* sphere = static_cast<Sphere*>(object);
                if (strEquals(key, "radius")) {
                    sphere->r = strToFloat(value);
                    sphere->r2 = powf(sphere->r, 2);
                }
            }
            else if (object->objectType == ObjectType::Plane) {
                Plane* plane = static_cast<Plane*>(object);
                if (strEquals(key, "normal")) {
                    plane->normal = str3ToFloat(splitString(value, ','));
                }
            }
            else if (object->objectType == ObjectType::Mesh) {
                Mesh* mesh = static_cast<Mesh*>(object);
                if (strEquals(key, "size")) {
                    mesh->size = str3ToFloat(splitString(value, ','));
                }
                else if (strEquals(key, "rot")) {
                    mesh->rot = str3ToFloat(splitString(value, ','));
                }
                else if (strEquals(key, "name")) {
                    mesh->loadOBJ(std::string(value), scene.options);
                }
            }
        }
    }

    options::combineWithGlobal(scene.options);

    ifs.close();
    return true;
}