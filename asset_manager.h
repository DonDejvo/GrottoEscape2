#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <fstream>
#include "asset_archive.h"

#ifndef _DEBUG
#define USE_ARCHIVE
#endif

class AssetLoadException : public std::runtime_error {
public:
    explicit AssetLoadException(const std::string& msg);
};

class AssetManager {
private:
#ifdef USE_ARCHIVE
    std::unordered_map<std::string, ArchiveEntry> mIndex;
    mutable std::ifstream mArchiveInputStream;
#else
    std::string mPrefix;
#endif
public:
    void Init();
    std::vector<char> Load(const std::string& path) const;
    std::string LoadToString(const std::string& path) const;
};