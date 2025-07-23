#pragma once

#include <fstream>
#include <iostream>

#include "Common.h"

namespace CHashing {

	// djb2 http://www.cse.yorku.ca/~oz/hash.html
	static uint32 getHash(const std::string& string) {
		uint32 Hash = 0;
		const unsigned char* str = (unsigned char*)string.c_str();

		int c;
		while ((c = *str++))
			Hash = ((Hash << 5) + Hash) + c;

		return Hash;
	}

	static uint32 getFileHash(const std::string& filePath){
		std::ifstream file(filePath, std::ios::ate | std::ios::binary);

		if (!file.is_open()){
			err("File Hashing could not find file {}", filePath.c_str());
		}

		// Get file size
		const auto fileSize = file.tellg();

		// Allocate memory to hold the entire file
		std::string memBlock;
		memBlock.resize(fileSize);

		// Read the file into memory
		file.seekg(0, std::ios::beg);
		file.read(memBlock.data(), static_cast<uint32>(memBlock.size()));
		file.close();

		// Get hash for the file
		const uint32 Hash = getHash(memBlock);

		return Hash;
	}
}
