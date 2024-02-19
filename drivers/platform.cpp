#include "platform.h"

#include <string>
#include <fmt/core.h>
#include <filesystem>
#include <assert.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <shlobj_core.h>
#endif

namespace lurp {

std::string GameFileToDir(const std::string& scriptFile)
{
    if (scriptFile.empty()) {
        return "default";
    }

    std::filesystem::path p = scriptFile;
    assert(p.has_stem());
    return p.stem().string();
}

std::ofstream OpenSaveStream(const std::string& path)
{
    std::ofstream stream;
    stream.open(path, std::ios::out);
    assert(stream.is_open());
	return stream;
}

std::ifstream OpenLoadStream(const std::string& path)
{
    std::ifstream stream;
    stream.open(path, std::ios::in);
    assert(stream.is_open());
    return stream;
}

bool CheckPath(const std::string& path, std::string& cwd)
{
    std::ofstream stream;
    stream.open(path, std::ios::in);
    if (!stream.is_open()) {
		fmt::print("ERROR: could not open file '{}'\n", path);
        cwd = std::filesystem::current_path().string();
        fmt::print("       Current working directory: '{}'\n", cwd);
		return false;
	}
    return true;
}

#ifdef _WIN32

void InitConsole()
{
    HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD oldMode = 0;
    //SetConsoleMode(hStdin, ENABLE_EXTENDED_FLAGS | ENABLE_QUICK_EDIT_MODE | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    GetConsoleMode(handle, &oldMode);
    DWORD mode = oldMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(handle, mode);
}

std::string OSSavePath()
{
    PWSTR path = NULL;
    HRESULT hres = SHGetKnownFolderPath(FOLDERID_SavedGames, 0, NULL, &path);
    if (SUCCEEDED(hres))
    {
        static constexpr int size = MAX_PATH * 2;
        char* buffer = new char[size];
        memset(buffer, 0, size);

        WideCharToMultiByte(CP_UTF8, 0, path, -1, buffer, size - 1, NULL, NULL);

        std::string converted_str(buffer);
        converted_str += "\\LuRP";

        CoTaskMemFree(path);
        delete[] buffer;
        return converted_str;
    }
    fmt::print("ERROR: could not find 'Saved Games' folder.\n");
    exit(-1);
}

int ConsoleWidth()
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
	return csbi.srWindow.Right - csbi.srWindow.Left + 1;
}

std::string SavePath(const std::string& dir, const std::string& stem, bool createDirs)
{
    std::string savePath = OSSavePath();
    savePath += "\\";
    savePath += dir;
    savePath += "\\";
    savePath += stem;
    savePath += ".lua";

    std::filesystem::path p(savePath);
    if (createDirs) {
		std::filesystem::create_directories(p.parent_path());
	}
    return savePath;
}

#else

#error Platform not defined.

#endif

} // namespace lurp
