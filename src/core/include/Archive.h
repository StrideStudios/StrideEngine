#pragma once

#include <vector>
#include <string>
#include <sstream>

#include "Common.h"

class CArchive {

public:
	virtual ~CArchive() = default;

	//
	// Strings
	// Since strings need to know how much to read from, they encode their size right before the actual string
	// Strings use the size_t value (which has a set size) so they know how much to read
	//

	friend CArchive& operator<<(CArchive& inArchive, const std::string& inValue) {
		inArchive << inValue.size();
		inArchive.write(inValue.data(), 1, inValue.size());
		return inArchive;
	}

	friend CArchive& operator>>(CArchive& inArchive, std::string& inValue) {
		size_t size;
		inArchive >> size;
		inValue.resize(size);
		inArchive.read(inValue.data(), 1, inValue.size());
		return inArchive;
	}

	//
	// Math
	//

	template <glm::length_t TSize, typename TType>
	friend CArchive& operator<<(CArchive& inArchive, const glm::vec<TSize, TType>& inVector) {
		for (glm::length_t i = 0; i < TSize; ++i) {
			inArchive << inVector[i];
		}
		return inArchive;
	}

	template <glm::length_t TSize, typename TType>
	friend CArchive& operator>>(CArchive& inArchive, glm::vec<TSize, TType>& inVector) {
		for (glm::length_t i = 0; i < TSize; ++i) {
			inArchive >> inVector[i];
		}
		return inArchive;
	}

	template <glm::length_t TCSize, glm::length_t TRSize, typename TType>
	friend CArchive& operator<<(CArchive& inArchive, const glm::mat<TCSize, TRSize, TType>& inMatrix) {
		for (glm::length_t c = 0; c < TCSize; ++c) {
			for (glm::length_t r = 0; r < TRSize; ++r) {
				inArchive << inMatrix[c][r];
			}
		}
		return inArchive;
	}

	template <glm::length_t TCSize, glm::length_t TRSize, typename TType>
	friend CArchive& operator>>(CArchive& inArchive, glm::mat<TCSize, TRSize, TType>& inMatrix) {
		for (glm::length_t c = 0; c < TCSize; ++c) {
			for (glm::length_t r = 0; r < TRSize; ++r) {
				inArchive >> inMatrix[c][r];
			}
		}
		return inArchive;
	}

	//
	// Vectors
	//

	template <typename TType, class TAlloc = std::allocator<TType>>
	friend CArchive& operator<<(CArchive& inArchive, const std::vector<TType, TAlloc>& inValue) {
		inArchive << inValue.size();
		for (auto value : inValue) {
			inArchive << value;
		}
		return inArchive;
	}

	// Vector assumes a default constructor and save overloads
	template <typename TType, class TAlloc = std::allocator<TType>>
	friend CArchive& operator>>(CArchive& inArchive, std::vector<TType, TAlloc>& inValue) {
		size_t size;
		inArchive >> size;
		inValue.resize(size);
		for (size_t i = 0; i < size; ++i) {
			inArchive >> inValue[i];
		}
		return inArchive;
	}

	//
	// Templated (works for basic types)
	//

	template <typename TType>
	friend CArchive& operator<<(CArchive& inArchive, const TType& inValue) {
		inArchive.write(reinterpret_cast<const char*>(&inValue), sizeof(TType), 1);
		return inArchive;
	}

	template <typename TType>
	friend CArchive& operator>>(CArchive& inArchive, TType& inValue) {
		inArchive.read(reinterpret_cast<char*>(&inValue), sizeof(TType), 1);
		return inArchive;
	}

protected:

	virtual void write(const void* inValue, size_t inElementSize, size_t inCount) = 0;

	virtual void read(void* inValue, size_t inElementSize, size_t inCount) = 0;

};

// An archive that can process files, uses standard c since it is faster
class CFileArchive final : public CArchive {

public:

	// If archive goes out of scope, close the file
	~CFileArchive() override {
		close();
	}

	CFileArchive(const char* inFilePath, const char* inMode) {
		fopen_s(&mFile, inFilePath, inMode);
		if (mFile) mIsOpen = true;
	}

	CFileArchive(const std::string& inFilePath, const char* inMode)
		: CFileArchive(inFilePath.c_str(), inMode) {}

	bool isOpen() const { return mIsOpen; }

	// Function to read from entire file with any type
	template <typename TType, class TAlloc = std::allocator<TType>>
	std::vector<TType, TAlloc> readFile(const bool inRemoveBOM = false) {
		assert(isOpen());

		fseek(mFile, 0L, SEEK_END);
		const auto fileSize = ftell(mFile);
		fseek(mFile, 0L, SEEK_SET);

		std::vector<TType, TAlloc> vector(fileSize / sizeof(TType));
		const size_t bytesRead = fread(vector.data(), sizeof(TType), vector.size(), mFile);

		// Since every part of the file is read, we might as well close it
		close();

		// The BOM might cause issues with certain interpreters
		if (inRemoveBOM) {
			static constexpr unsigned char BOM[] = { 0xEF, 0xBB, 0xBF };
			if (bytesRead > 3) {
				if (!memcmp(vector.data(), BOM, 3))
					memset(vector.data(), ' ', 3);
			}
		}

		return vector;
	}

	// Read into char vector, then reinterpret to a string
	std::string readFile(const bool inRemoveBOM = false) {
		std::vector<char> vector = readFile<char>(inRemoveBOM);
		return {vector.data(), vector.size()};
	}

	// Function to write to entire file with any type
	template <typename TType, class TAlloc = std::allocator<TType>>
	void writeFile(std::vector<TType, TAlloc> vector) {
		assert(isOpen());
		fwrite(vector.data(), sizeof(TType), vector.size(), mFile);

		// Since every part of the file is read, we might as well close it
		close();
	}

	// Char has size of 1
	void writeFile(const std::string &string) {
		assert(isOpen());
		fwrite(string.data(), 1, string.size(), mFile);

		// Since every part of the file is read, we might as well close it
		close();
	}

	void close() {
		if (isOpen()) {
			fclose(mFile);
			mIsOpen = false;
		}
	}

protected:

	void write(const void* inValue, const size_t inElementSize, const size_t inCount) override {
		assert(isOpen());
		fwrite(inValue, inElementSize, inCount, mFile);
	}

	void read(void* inValue, size_t const inElementSize, const size_t inCount) override {
		assert(isOpen());
		fread(inValue, inElementSize, inCount, mFile);
	}

private:

	FILE* mFile;
	bool mIsOpen = false;

};