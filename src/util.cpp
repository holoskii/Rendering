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

Mesh* loadOBJ(const std::string& filename, const Vec3f& pos, const Vec3f& size, const Options& options)
{
    auto getUInt = [](const char* & ptr)
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
    if (!ifs.good()) return nullptr;

    Mesh* mesh = new Mesh();
    mesh->ac = std::make_unique<AccelerationStructure>();
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
        std::getline(ifs, line);
        if (line.find('#') != std::string::npos) line.erase(line.find('#'));
        if (line.length() <= 0) continue;

        const char* c_line = line.c_str();
        char lineHeader[32] = { 0 };
        int res = sscanf_s(c_line, "%s", lineHeader, 32u);
        if (res == 0) {
            delete mesh;
            return nullptr;
        }
        c_line += strlen(lineHeader) + 1;

        if (strcmp(lineHeader, "v") == 0) {
            float x, y, z;
            int res = sscanf(c_line, "%f %f %f", &x, &y, &z);
            assert(res == 3);
            min.x = std::min(x, min.x); min.y = std::min(y, min.y);
            min.z = std::min(z, min.z); max.x = std::max(x, max.x);
            max.y = std::max(y, max.y); max.z = std::max(z, max.z);
            vertexData.emplace_back(Vec3f(x, y, z));
        }
        else if (strcmp(lineHeader, "vn") == 0) {
            float x, y, z;
            int res = sscanf_s(c_line, "%f %f %f", &x, &y, &z);
            assert(res == 3);
            normalData.emplace_back(Vec3f{ x, y, z }.normalize());
        }
        else if (strcmp(lineHeader, "f") == 0) {
            
            if (!normalized) {
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
                    v.x = normSize.x * ((v.x - min.x) / range.x - 0.5f) + pos.x;
                    v.y = normSize.y * ((v.y - min.y) / range.y - 0.5f) + pos.y;
                    v.z = normSize.z * ((v.z - min.z) / range.z - 0.5f) + pos.z;
                }
                mesh->ac->setBounds(pos - normSize / 2, pos + normSize / 2);
            }

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
    
    mesh->allTris.reserve(tris.size());
    for (const Triangle* tri : tris)
        mesh->allTris.push_back(tri);
    mesh->ac->setup(tris, 1, options);
#ifdef _STATS
    stats::meshCount.store(stats::meshCount.load() + mesh->allTris.size());
#endif // _STATS
    return mesh;
}

bool loadScene(Scene& scene, Options& options, const std::string& sceneName)
{
#ifndef _NO_OUTPUT
	std::cout << "Loading scene " << sceneName << '\n'; 
#endif // _NO_OUTPUT
	
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
			if (str.find('#') != std::string::npos)
				str.erase(str.find('#'));
			if (str.length() == 0)
				continue;

			if (str[0] == '[') {
				if (str.find("end") != std::string::npos) {
					ifs.close();
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
					else if (strv.find("image_name") != std::string::npos) {
						std::string name = str.substr(str.find('=') + 1);
						if (name[0] == '\"') name.erase(0, 1);
						if (name[name.length() - 1] == '\"') name.erase(name.length() - 1, 1);
						options.imageName = name;
					}
					else if (strv.find("n_workers=") != std::string::npos)
						options.nWorkers = strToInt(str.substr(str.find('=') + 1));
					else if (strv.find("max_ray_depth") != std::string::npos)
						options.maxRayDepth = strToInt(str.substr(str.find('=') + 1));
					else if (strv.find("ac_max_depth") != std::string::npos)
						options.maxDepthAccelStruct = strToInt(str.substr(str.find('=') + 1));
					else if (strv.find("ac_min_batch") != std::string::npos)
						options.minBatchSizeAccelStruct = strToInt(str.substr(str.find('=') + 1));
					else if (strv.find("position") != std::string::npos) {
						std::vector<std::string> res;
						std::stringstream lineStream(str.substr(str.find('=') + 1));
						std::string cell;
						while (std::getline(lineStream, cell, ','))
							res.push_back(cell);
						assert(res.size() == 3);
						Vec3f pos = { strToFloat(res[0]), strToFloat(res[1]), strToFloat(res[2]) };
						scene.camera.pos = pos;
					}
					else if (strv.find("rotation") != std::string::npos) {
						std::vector<std::string> res;
						std::stringstream lineStream(str.substr(str.find('=') + 1));
						std::string cell;
						while (std::getline(lineStream, cell, ','))
							res.push_back(cell);
						assert(res.size() == 3);
						Vec3f rot = { strToFloat(res[0]), strToFloat(res[1]), strToFloat(res[2]) };
						scene.camera.rot = rot;
					}
				}
				else if (blockType == BlockType::Light) {
					options::combineWithGlobal(options);
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
						scene.lights.emplace_back(std::unique_ptr<Light>(light));
					}
					else if (res[1].find("point_light") != std::string::npos) {
						Vec3f pos{ strToFloat(res[2]), strToFloat(res[3]), strToFloat(res[4]) };
						Vec3f color{ strToFloat(res[5]), strToFloat(res[6]), strToFloat(res[7]) };
						PointLight* light = new PointLight(pos, color, strToFloat(res[8]));
						scene.lights.emplace_back(std::unique_ptr<Light>(light));
					}
					else {
						std::cout << "Unknown light type\n";
						break;
					}
				}
				else if (blockType == BlockType::Object) {
					options::combineWithGlobal(options);
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
						Vec3f color{ strToFloat(res[9]), strToFloat(res[10]), strToFloat(res[11]) };
						object = static_cast<Object*>(loadOBJ(path, pos, size, options));
						object->color = color;
						if (object == nullptr) {
							std::cout << "Mesh " << path << " wasn't loaded\n";
							exit(-1);
						}
						i = 12;
					}
					else {
						std::cout << "Unknown object type\n";
						break;
					}

					if (i < res.size()) {
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
					}

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

					scene.objects.emplace_back(std::unique_ptr<Object>(object));
				}
			}
		}
		std::getline(ifs, str);
	}
	std::cout << "Scene wasn't loaded\n";
	return false;
}

int recInterAC(const Ray& ray, AccelerationStructure* ac)
{
	if (ac == nullptr)
		return 0;
	if (ac->intersectBox(ray))
		return 1 + recInterAC(ray, ac->left.get()) + recInterAC(ray, ac->right.get());
}

int interAC(const Ray& ray, const ObjectVector& objects, const Options& options)
{
	int sum = 0;
	for (auto& obj : objects) {
		if (obj->objectType == ObjectType::Mesh) {
			Mesh* mesh = dynamic_cast<Mesh*>(obj.get());
			sum += recInterAC(ray, mesh->ac.get());
		}
	}
	return sum;
}
