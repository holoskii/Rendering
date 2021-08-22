// utility functions and functions working with file IO
#include "util.h"

#ifdef _WIN32
    #define NOMINMAX
    #include "windows.h"
    #include "shellapi.h"
#endif // _WIN32

#include <fstream>

#include "options.h"

int saveImage(Vec3f* frameBuffer, const Options& options)
{
    std::string path = options.rootPath + "\\" + options.imageName + std::string(".ppm");
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
#ifdef _WIN32
    wchar_t* wPath = new wchar_t[strlen(path.c_str()) + 1];
    mbstowcs(wPath, path.c_str(), strlen(path.c_str()) + 1);
    ShellExecute(NULL, L"open", wPath, NULL, NULL, SW_SHOWDEFAULT);
    delete[] wPath;
#endif // _WIN32
    delete[] data;
    return 0;
}

unsigned char* loadBMP(const char* filename, int& width, int& height)
{
    int i;
    FILE* f = fopen(filename, "rb");
    if (f == NULL) LOG_ERROR
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