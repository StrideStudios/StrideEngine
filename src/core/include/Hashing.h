#pragma once

#include <fstream>

#include "Archive.h"
#include "Common.h"

// Simple hashing, could potentially have repeats, but it is unlikely
// djb2 http://www.cse.yorku.ca/~oz/hash.html
static uint32 getHash(const std::string& string) {
	uint32 Hash = 0;
	const unsigned char* str = (unsigned char*)string.c_str();

	int c;
	while ((c = *str++))
		Hash = ((Hash << 5) + Hash) + c;

	return Hash;
}

template <typename TType>
static uint32 getHash(const std::shared_ptr<TType>& inSharedPointer) {
	return getHash(*inSharedPointer);
}

template <typename TType>
static uint32 getHash(const std::unique_ptr<TType>& inSharedPointer) {
	return getHash(*inSharedPointer);
}

static uint32 getFileHash(const std::string& filePath){
	CFileArchive file(filePath, "rb");

	if (!file.isOpen()){
		errs("File Hashing could not find file {}", filePath.c_str());
	}

	// Get hash for the file
	const uint32 Hash = getHash(file.readFile());

	return Hash;
}