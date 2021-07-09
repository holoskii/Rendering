#include "util.h"
#include "timer.h"

#define NOMINMAX
#include "windows.h"
#include "shellapi.h"
#include <fstream>

int savePPM(Vec3f* frameBuffer, const Options& options)
{
    Timer t("Save image");

    // intermediate buffer significantly speeds up file write process
    char* data = new char[options.height * options.width * 3];
    char* dataPtr = data;
    for (int i = 0; i < options.height * options.width; i++)
        for (int j = 0; j < 3; j++)
            *dataPtr++ = static_cast<char>(clamp(0.0f, 1.0f, frameBuffer[i][j]) * 255);

    std::string path = options.imagePath + "\\" + options.imageName;
    std::ofstream of(path, std::ios::out | std::ios::binary);
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
