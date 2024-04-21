#include "platform.h"

#include <string>
#include <fmt/core.h>
#include <filesystem>
#include <assert.h>
#include <plog/Log.h>

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
        PLOG(plog::error) << fmt::format("Could not open file '{}'", path);
        cwd = std::filesystem::current_path().string();
        PLOG(plog::error) << fmt::format("Current working directory: '{}'", cwd);

        if (path.substr(path.size() - 9) == "_test.lua") {
            PLOG(plog::warning) << fmt::format("Basic tests failed. This is likely due to running from the incorrect directory.");
            PLOG(plog::warning) << fmt::format("The working directory should be the root of the LuRP.");
        }
        exit(-1);
        return false;
    }
    return true;
}

std::vector<std::filesystem::path> ScanGameFiles()
{
    std::vector<std::filesystem::path> gameFiles;
    try {
        for (const auto& entry : std::filesystem::directory_iterator("game")) {
            if (entry.is_directory()) {
                std::string dirName = entry.path().filename().string();
                std::filesystem::path pathStem = dirName + ".lua";
                std::filesystem::path gamePath = entry / pathStem;
                if (std::filesystem::exists(gamePath))
                    gameFiles.push_back(gamePath);
            }
        }
    }
    catch (const std::filesystem::filesystem_error& e) {
        PLOG(plog::error) << fmt::format("Error scanning game files: {}", e.what());
    }
    return gameFiles;
}

#ifdef _WIN32

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
    PLOG(plog::error) << "Could not find 'Saved Games' folder.";
    exit(-1);
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

std::string LogPath()
{
    std::string logPath = OSSavePath();
    logPath += "\\";
    logPath += "lurp.txt";
    return logPath;
}

#elif __APPLE__

std::string OSSavePath()
{
    return "~/Library/Application Support/LuRP";
}

std::string SavePath(const std::string& dir, const std::string& stem, bool createDirs)
{
    std::string savePath = OSSavePath();
    savePath += "/";
    savePath += dir;
    savePath += "/";
    savePath += stem;
    savePath += ".lua";

    std::filesystem::path p(savePath);
    if (createDirs) {
        std::filesystem::create_directories(p.parent_path());
    }
    return savePath;
}

std::string LogPath()
{
    std::string logPath = OSSavePath();
    logPath += "/";
    logPath += "lurp.txt";
    return logPath;
}


#else

#error Platform not defined.

#endif

} // namespace lurp
