#pragma once

#include <cstdint>

const size_t MAX_PATH_LEN = 128;

struct ArchiveEntry {
	char path[MAX_PATH_LEN] = {};
	uint64_t offset = 0;
	uint64_t size = 0;
};