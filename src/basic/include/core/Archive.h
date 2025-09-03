#pragma once

#include <set>
#include <unordered_set>
#include <map>
#include <unordered_map>
#include <utility>
#include <vector>
#include <string>
#include <memory>

#include "control/ClassManager.h"

class CArchive;

// A serializable function that can be passed down
class ISerializable {
public:
	virtual CArchive& save(CArchive& inArchive) = 0;

	virtual CArchive& load(CArchive& inArchive) = 0;
};

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

	friend CArchive& operator<<(CArchive& inArchive, const Transform2f& inTransform) {
		inArchive << inTransform.getOrigin();
		inArchive << inTransform.getPosition();
		inArchive << inTransform.getRotation();
		inArchive << inTransform.getScale();
		return inArchive;
	}

	friend CArchive& operator>>(CArchive& inArchive, Transform2f& inTransform) {
		Vector2f origin;
		Vector2f position;
		float rotation;
		Vector2f scale;
		inArchive >> origin;
		inArchive >> position;
		inArchive >> rotation;
		inArchive >> scale;
		inTransform.setOrigin(origin);
		inTransform.setPosition(position);
		inTransform.setRotation(rotation);
		inTransform.setScale(scale);
		return inArchive;
	}

	friend CArchive& operator<<(CArchive& inArchive, const Transform3f& inTransform) {
		inArchive << inTransform.getPosition();
		inArchive << inTransform.getRotation();
		inArchive << inTransform.getScale();
		return inArchive;
	}

	friend CArchive& operator>>(CArchive& inArchive, Transform3f& inTransform) {
		Vector3f position;
		Vector3f rotation;
		Vector3f scale;
		inArchive >> position;
		inArchive >> rotation;
		inArchive >> scale;
		inTransform.setPosition(position);
		inTransform.setRotation(rotation);
		inTransform.setScale(scale);
		return inArchive;
	}

	//
	// Sets
	//

	template <typename TType>
	friend CArchive& operator<<(CArchive& inArchive, const std::set<TType>& inValue) {
		inArchive << inValue.size();
		for (auto value : inValue) {
			inArchive << value;
		}
		return inArchive;
	}

	template <typename TType>
	friend CArchive& operator>>(CArchive& inArchive, std::set<TType>& inValue) {
		size_t size;
		inArchive >> size;
		for (size_t i = 0; i < size; ++i) {
			TType value;
			inArchive >> value;
			inValue.emplace(value);
		}
		return inArchive;
	}

	template <typename TType>
	friend CArchive& operator<<(CArchive& inArchive, const std::unordered_set<TType>& inValue) {
		inArchive << inValue.size();
		for (auto value : inValue) {
			inArchive << value;
		}
		return inArchive;
	}

	template <typename TType>
	friend CArchive& operator>>(CArchive& inArchive, std::unordered_set<TType>& inValue) {
		size_t size;
		inArchive >> size;
		for (size_t i = 0; i < size; ++i) {
			TType value;
			inArchive >> value;
			inValue.emplace(value);
		}
		return inArchive;
	}

	//
	// Maps
	//

	template <typename TKeyType, typename TValueType>
	friend CArchive& operator<<(CArchive& inArchive, const std::map<TKeyType, TValueType>& inValue) {
		inArchive << inValue.size();
		for (auto pair : inValue) {
			inArchive << pair.first;
			inArchive << pair.second;
		}
		return inArchive;
	}

	template <typename TKeyType, typename TValueType>
	friend CArchive& operator>>(CArchive& inArchive, std::map<TKeyType, TValueType>& inValue) {
		size_t size;
		inArchive >> size;
		for (size_t i = 0; i < size; ++i) {
			// Get key values
			TKeyType first;
			TValueType second;
			inArchive >> first;
			inArchive >> second;

			// Set key value pair
			inValue[first] = second;
		}
		return inArchive;
	}

	template <typename TKeyType, typename TValueType>
	friend CArchive& operator<<(CArchive& inArchive, const std::unordered_map<TKeyType, TValueType>& inValue) {
		inArchive << inValue.size();
		for (auto pair : inValue) {
			inArchive << pair.first;
			inArchive << pair.second;
		}
		return inArchive;
	}

	template <typename TKeyType, typename TValueType>
	friend CArchive& operator>>(CArchive& inArchive, std::unordered_map<TKeyType, TValueType>& inValue) {
		size_t size;
		inArchive >> size;
		for (size_t i = 0; i < size; ++i) {
			// Get key values
			TKeyType first;
			TValueType second;
			inArchive >> first;
			inArchive >> second;

			// Set key value pair
			inValue[first] = second;
		}
		return inArchive;
	}

	//
	// Vectors
	//
	//TODO: inherited classes DO NOT work
	template <typename TType>
	friend CArchive& operator<<(CArchive& inArchive, const std::vector<TType>& inValue) {
		inArchive << inValue.size();
		for (auto value : inValue) {
			inArchive << value;
		}
		return inArchive;
	}

	// Vector assumes a default constructor and save overloads
	template <typename TType>
	friend CArchive& operator>>(CArchive& inArchive, std::vector<TType>& inValue) {
		size_t size;
		inArchive >> size;
		inValue.resize(size);
		for (size_t i = 0; i < size; ++i) {
			inArchive >> inValue[i];
		}
		return inArchive;
	}

	//
	// Arrays
	//

	template <typename TType, size_t _Size>
	friend CArchive& operator<<(CArchive& inArchive, const std::array<TType, _Size>& inValue) {
		for (auto value : inValue) {
			inArchive << value;
		}
		return inArchive;
	}

	// Since arrays have a set size, it does not need to be added to the save data
	template <typename TType, size_t _Size>
	friend CArchive& operator>>(CArchive& inArchive, std::array<TType, _Size>& inValue) {
		for (size_t i = 0; i < _Size; ++i) {
			inArchive >> inValue[i];
		}
		return inArchive;
	}

	//
	// Pointers need to be dereferenced on write and constructed on read
	// Polymorphic types for shared_ptr must use REGISTER_CLASS so the type can be created
	//

	template <typename TType>
	requires (not std::is_polymorphic_v<TType>) or std::is_default_constructible_v<typename TType::Class>
	friend CArchive& operator<<(CArchive& inArchive, const std::shared_ptr<TType>& inValue) {
		if constexpr (std::is_polymorphic_v<TType>) {
			inArchive << inValue->getClass().getName();
		}
		inArchive << *inValue;
		return inArchive;
	}

	template <typename TType>
	requires std::is_default_constructible_v<TType> and ((not std::is_polymorphic_v<TType>) or std::is_default_constructible_v<typename TType::Class>)
	friend CArchive& operator>>(CArchive& inArchive, std::shared_ptr<TType>& inValue) {
		if constexpr (std::is_polymorphic_v<TType>) {
			std::string className;
			inArchive >> className;
			inValue = std::static_pointer_cast<TType>(CClassManager::construct(className.c_str()));
		} else {
			inValue = std::make_shared<TType>();
		}
		inArchive >> *inValue;
		return inArchive;
	}

	template <typename TType>
	friend CArchive& operator<<(CArchive& inArchive, const std::unique_ptr<TType>& inValue) {
		inArchive << *inValue;
		return inArchive;
	}

	template <typename TType>
	requires std::is_default_constructible_v<TType>
	friend CArchive& operator>>(CArchive& inArchive, std::unique_ptr<TType>& inValue) {
		inValue = std::make_unique<TType>();
		inArchive >> *inValue;
		return inArchive;
	}

	//
	// Serializable Types
	// Useful because save and load are virtual
	//

	friend CArchive& operator<<(CArchive& inArchive, ISerializable& inValue) {
		return inValue.save(inArchive);
	}

	friend CArchive& operator>>(CArchive& inArchive, ISerializable& inValue) {
		return inValue.load(inArchive);
	}

	//
	// Templated (for basic types)
	// Makes sure they are trivial
	//

	template <typename TType>
	requires std::is_trivial_v<TType>
	friend CArchive& operator<<(CArchive& inArchive, const TType& inValue) {
		inArchive.write(&inValue, sizeof(TType), 1);
		return inArchive;
	}

	template <typename TType>
	requires std::is_trivial_v<TType>
	friend CArchive& operator>>(CArchive& inArchive, TType& inValue) {
		inArchive.read(&inValue, sizeof(TType), 1);
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
	virtual ~CFileArchive() override {
		close();
	}

	CFileArchive(const char* inFilePath, const char* inMode) {
		fopen_s(&mFile, inFilePath, inMode);
		if (mFile) mIsOpen = true;
	}

	CFileArchive(const std::string& inFilePath, const char* inMode)
		: CFileArchive(inFilePath.c_str(), inMode) {}

	no_discard bool isOpen() const { return mIsOpen; }

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

	virtual void write(const void* inValue, const size_t inElementSize, const size_t inCount) override {
		assert(isOpen());
		fwrite(inValue, inElementSize, inCount, mFile);
	}

	virtual void read(void* inValue, size_t const inElementSize, const size_t inCount) override {
		assert(isOpen());
		fread(inValue, inElementSize, inCount, mFile);
	}

private:

	FILE* mFile;
	bool mIsOpen = false;

};