#pragma once

#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <filesystem>
#include <optional>

namespace lurp {

// --- Platform-specific functions ---
// Given dir="default" and stem="auto" return "c:/user/thedude/Saved Games/default/auto.lua"
std::filesystem::path SavePath(const std::string& dir, const std::string& stem, bool createDirectories = true);
std::filesystem::path LogPath(const std::string& stem);

// --- General functions ---
// Gets the 'stem' for saving from the path to the game file.
std::string GameFileToDir(const std::string& gameFile);
std::ofstream OpenSaveStream(const std::filesystem::path& path);
std::ifstream OpenLoadStream(const std::filesystem::path& path);

std::filesystem::path ConstructAssetPath(const std::vector<std::filesystem::path>& assetsDirs, const std::vector<std::string>& files);

bool CheckPath(const std::string& path, std::string& cwd);

std::vector<std::filesystem::path> ScanGameFiles();

} // namespace lurp