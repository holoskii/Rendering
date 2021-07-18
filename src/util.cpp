#include "util.h"

#define NOMINMAX
#include "windows.h"
#include "shellapi.h"
#include <fstream>
#include <algorithm>
#include <map>
#include <string>

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
        for (int j = 0; j < 3; j++)
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

Mesh* loadOBJ(const std::string& filename, const Vec3f& pos, const Vec3f& size)
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
    mesh->accelStruct = new AccelerationStructure();
    std::string line;
    bool normalized = false;
    std::vector<Vec3f> vertexData;
    std::vector<Vec3f> normalData;
    std::vector<const Triangle*>* tris = new std::vector<const Triangle*>;
    Vec3f min = { std::numeric_limits<float>::max() };
    Vec3f max = { std::numeric_limits<float>::min() };

    std::cout << "Mesh: " << filename << '\n';
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
                Vec3f normSize = size;
                normSize.y = normSize.x / ((max.x - min.x) / (max.y - min.y));
                normSize.z = normSize.x / ((max.x - min.x) / (max.z - min.z));
                Vec3f range = max - min;
                for (auto& v : vertexData) {
                    v.x = normSize.x * ((v.x - min.x) / range.x - 0.5f) + pos.x;
                    v.y = normSize.y * ((v.y - min.y) / range.y - 0.5f) + pos.y;
                    v.z = normSize.z * ((v.z - min.z) / range.z - 0.5f) + pos.z;
                }

                mesh->accelStruct->setBounds(pos - normSize / 2, pos + normSize / 2);
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
                    tris->push_back(new Triangle(vertexData.at(vi.at(0) - 1), vertexData.at(vi.at(i) - 1), vertexData.at(vi.at(i + 1) - 1)));
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
                        tris->push_back(new Triangle(vertexData.at(vi.at(0) - 1), vertexData.at(vi.at(i) - 1), vertexData.at(vi.at(i + 1) - 1)));
                }
                else {
                    assert(ni.size() == vi.size());
                    for (size_t i = 1; i < vi.size() - 1; i++)
                        tris->push_back(new Triangle(vertexData.at(vi.at(0) - 1), vertexData.at(vi.at(i) - 1), vertexData.at(vi.at(i + 1) - 1),
                            normalData.at(ni.at(0) - 1), normalData.at(ni.at(i) - 1), normalData.at(ni.at(i + 1) - 1)));
                }
            }
            else {
                std::cout << "unhandled slash count: " << slashCount << '\n';
            }
        }
    } while (ifs.good());
    ifs.close();

    mesh->allTris.reserve(tris->size());
    for (const Triangle*& tri : *tris)
        mesh->allTris.push_back(tri);
    mesh->accelStruct->setTris(tris);
#ifdef _STATS
    std::cout << "Triangle copies: " << triCopiesCount.load() << ", triangles in mesh: " << mesh->allTris.size() << '\n';
#endif // _STATS
    return mesh;
}
