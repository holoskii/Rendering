// utility functions and functions working with file IO
#include "util.h"

#ifdef _WIN32
    #define NOMINMAX
    #include "windows.h"
    #include "shellapi.h"
#endif // _WIN32

#include <fstream>
#include <cstring>

#include "options.h"

int saveImage(Vec3f* frameBuffer, const Options& options)
{
    std::string path = options.imageName + std::string(".bmp");
    std::ofstream of(path, std::ios::out | std::ios::binary);
    if (!of.good()) {
        std::cout << "Could not open output file " << path << '\n';
        return -1;
    }
    
    std::cout << "Successfully wrote to output file " << path << '\n';

    const size_t headerSize = 54;
    char header[headerSize] = { 0 };
    int paddingBytes = options.width % 4 == 0 ? 0 : 4 - options.width * 3 % 4;
    size_t arraySize = options.height * (options.width + paddingBytes) * 3;
    size_t totalSize = headerSize + arraySize;

    
    memcpy(header, "BM", 2);
    *(size_t*)(header + 0x2) = totalSize;
    *(size_t*)(header + 0xA) = headerSize;
    *(size_t*)(header + 0xE) = headerSize - 14;
    *(size_t*)(header + 0x12) = options.width;
    *(size_t*)(header + 0x16) = options.height;
    *(header + 0x1A) = 1;
    *(header + 0x1C) = 24;
    *(size_t*)(header + 0x22) = arraySize;
    *(size_t*)(header + 0x26) = 2835;
    *(size_t*)(header + 0x2A) = 2835;

    // intermediate buffer significantly speeds up file write process
    char* data = new char[options.height * options.width * 3];
    char* dataPtr = data;
    for (size_t i = 0; i < options.height; i++) {
        for (size_t j = 0; j < options.width; j++) {
            size_t v = (options.height - 1 - i) * options.width + j;
            for (int k = 2; k >= 0; k--) {
                *dataPtr++ = static_cast<char>(clamp(0.0f, 1.0f, frameBuffer[v][k]) * 255);
            }
        }
        for (int j = 0; j < paddingBytes; j++) {
            dataPtr++;
        }
    }

    of.write(header, headerSize);
    of.write(data, arraySize);
    of.close();

    std::string fullPath = (std::filesystem::current_path() / path).string();
    #ifdef _WIN32
        wchar_t* wPath = new wchar_t[strlen(fullPath.c_str()) + 1];
        mbstowcs(wPath, fullPath.c_str(), strlen(fullPath.c_str()) + 1);
        ShellExecute(NULL, L"open", wPath, NULL, NULL, SW_SHOWDEFAULT);
        delete[] wPath;
    #elif __linux__
        auto linuxCmd = "xdg-open " + fullPath;
        system(linuxCmd.c_str());
    #endif
    delete[] data;
    return 0;
}

unsigned char* loadBMP(const char* filename, int& width, int& height)
{
    int i;
    FILE* f = fopen(filename, "rb");
    if (f == NULL) {
        std::cout << "Could not open .bmp file: " << filename << '\n';
        LOG_ERROR();
        return NULL;
    }
    unsigned char info[54];

    // read the 54-byte header
    fread(info, sizeof(unsigned char), 54, f);

    // extract image height and width from header
    width = *(int*)&info[18];
    height = *(int*)&info[22];

    // allocate 3 bytes per pixel
    int size = 3 * width * height;
    unsigned char* data = new unsigned char[size];

    // read the rest of the data at once
    fread(data, sizeof(unsigned char), size, f);
    fclose(f);

    for (i = 0; i < size; i += 3)
    {
        // flip the order of every 3 bytes
        unsigned char tmp = data[i];
        data[i] = data[i + 2];
        data[i + 2] = tmp;
    }

    return data;
}
