// utility functions and functions working with file IO
#include "util.h"

#ifdef _WIN32
    #define NOMINMAX
    #include "windows.h"
    #include "shellapi.h"
#endif // _WIN32

#include <fstream>

#include "options.h"

int savePPM(Vec3f* frameBuffer, const Options& options)
{
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
#ifdef _WIN32
    wchar_t* wPath = new wchar_t[strlen(path.c_str()) + 1];
    mbstowcs(wPath, path.c_str(), strlen(path.c_str()) + 1);
    ShellExecute(NULL, L"open", wPath, NULL, NULL, SW_SHOWDEFAULT);
    delete[] wPath;
#endif // _WIN32
    delete[] data;
    return 0;
}
