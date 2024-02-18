#pragma once

#include <string>
#include <iostream>
#include <fstream>

namespace lurp {

// Given dir="default" and stem="auto" return "c:/user/thedude/Saved Games/default/auto.lua"
std::string SavePath(const std::string& dir, const std::string& stem, bool createDirectories = true);

// Gets the 'stem' for saving from the path to the game file.
std::string GameFileToDir(const std::string& gameFile);

std::ofstream OpenSaveStream(const std::string& path);
std::ifstream OpenLoadStream(const std::string& path);

bool CheckPath(const std::string& path, std::string& cwd);

int ConsoleWidth();

} // namespace lurp