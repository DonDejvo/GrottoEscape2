#include <iostream>
#include <filesystem>
#include <vector>
#include <fstream>
#include "asset_archive.h"

namespace fs = std::filesystem;

const size_t CHUNK_SIZE = 1 << 16;

struct SourceFile {
	std::string virtualPath;
	std::filesystem::path fullPath;
	uint64_t size = 0;
};

static std::string ToVirtualPath(const fs::path& fullPath, const fs::path& basePath) {
	fs::path relPath = fs::relative(fullPath, basePath);
	return relPath.generic_string();
}

int main(int argc, char** argv) {
	if (argc != 3) {
		std::cerr << "Usage: " << argv[0] << " <source_dir> <output_path>\n";
		return 1;
	}

	fs::path sourceDir = argv[1];
	fs::path outputPath = argv[2];

	if (!fs::is_directory(sourceDir)) {
		std::cerr << "Error: Source directory does not exist: " << sourceDir << "\n";
		return 1;
	}

	std::vector<SourceFile> sourceFiles;

	for (const auto& entry : fs::recursive_directory_iterator(sourceDir)) {
		if (!entry.is_regular_file()) continue;

		SourceFile source;
		source.fullPath = entry.path();
		source.size = entry.file_size();
		source.virtualPath = ToVirtualPath(source.fullPath, sourceDir);

		if (source.virtualPath.size() >= MAX_PATH_LEN) {
			std::cerr << "Error: path too long (max " << (MAX_PATH_LEN - 1) << " characters): " << source.virtualPath << "\n";
			return 1;
		}

		sourceFiles.push_back(std::move(source));
	}

	std::cout << "Found " << sourceFiles.size() << " assets under " << sourceDir << "\n";

	std::vector<ArchiveEntry> archiveEntries(sourceFiles.size());

	int32_t entryCount = sourceFiles.size();
	uint64_t offset = sizeof(int32_t) + sizeof(ArchiveEntry) * archiveEntries.size();

	for (int i = 0; i < entryCount; ++i) {
		ArchiveEntry& entry = archiveEntries[i];

		memset(entry.path, 0, MAX_PATH_LEN);
		memcpy(entry.path, sourceFiles[i].virtualPath.data(), sourceFiles[i].virtualPath.size());
		entry.offset = offset;
		entry.size = sourceFiles[i].size;

		offset += entry.size;
	}

	std::ofstream os(outputPath, std::ios::binary | std::ios::trunc);

	if (!os.is_open()) {
		std::cerr << "Error: Could not open output file: " << outputPath << "\n";
		return 1;
	}

	os.write(reinterpret_cast<char*>(&entryCount), sizeof(entryCount));
	os.write(reinterpret_cast<char*>(archiveEntries.data()), sizeof(ArchiveEntry) * archiveEntries.size());

	std::vector<char> chunk(CHUNK_SIZE);

	for (const auto& source : sourceFiles) {
		std::ifstream is(source.fullPath, std::ios::binary);
		if (!is.is_open()) {
			std::cerr << "Error: Could not open source file: " << source.fullPath << "\n";
			return 1;
		}

		uint64_t remainingSize = source.size;

		while (remainingSize > 0) {
			uint64_t readSize = std::min(remainingSize, CHUNK_SIZE);

			is.read(chunk.data(), readSize);
			os.write(chunk.data(), readSize);

			remainingSize -= readSize;
		}

		std::cout << "packed: " << source.virtualPath << " (" << source.size << " bytes)\n";
	}

	os.close();

	std::cout << "Done. Archive written to " << outputPath << "\n";

	return 0;
}