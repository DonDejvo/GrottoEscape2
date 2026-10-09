#include "asset_manager.h"
#include <iostream>
#include <fstream>
#include <filesystem>

AssetLoadException::AssetLoadException(const std::string& msg) 
    : std::runtime_error(msg)
{
}

#ifdef USE_ARCHIVE

void AssetManager::Init() {
    std::string archivePath = "data/game.dat";

    mArchiveInputStream.open(archivePath, std::ios::binary);

    if (!mArchiveInputStream.is_open()) {
        throw AssetLoadException("Could not open archive: " + archivePath);
    }

    int32_t entryCount = 0;

    mArchiveInputStream.read(reinterpret_cast<char*>(&entryCount), sizeof(entryCount));

    std::vector<ArchiveEntry> archiveEntries(entryCount);

    mArchiveInputStream.read(reinterpret_cast<char*>(archiveEntries.data()), entryCount * sizeof(ArchiveEntry));

    for (const auto& entry : archiveEntries) {
        mIndex[entry.path] = entry;
    }
}

std::vector<char> AssetManager::Load(const std::string& path) const {
    auto it = mIndex.find(path);

    if (it == mIndex.end()) {
        throw AssetLoadException("Could not load asset: " + path);
    }

    const ArchiveEntry& entry = it->second;

    std::vector<char> buffer(entry.size);

    mArchiveInputStream.seekg(entry.offset);
    mArchiveInputStream.read(buffer.data(), entry.size);

    return buffer;
}

#else

void AssetManager::Init() {
    mPrefix = "assets";
}

std::vector<char> AssetManager::Load(const std::string& path) const {
    std::filesystem::path filename = mPrefix;
    filename /= path;

    std::ifstream ifs(filename, std::ios::binary | std::ios::ate);

    if (!ifs.is_open()) {
        throw AssetLoadException("Could not load asset: " + path);
    }

    std::ifstream::pos_type pos = ifs.tellg();

    std::vector<char> buffer(pos);

    ifs.seekg(0, std::ios::beg);
    ifs.read(&buffer[0], pos);

    return buffer;
}

#endif

std::string AssetManager::LoadToString(const std::string& path) const {
    std::vector<char> buffer = Load(path);
    return std::string(buffer.data(), buffer.size());
}