#pragma once

#include <glm/glm.hpp>

typedef unsigned char uint8;
typedef signed char int8;
typedef unsigned short uint16;
typedef signed short int16;
typedef unsigned int uint32;
typedef signed int int32;
typedef unsigned long long uint64;
typedef signed long long int64;

// Extent should always be used with integer types

typedef glm::vec<2, int32> Extent32;
typedef glm::vec<2, uint32> Extent32u;
typedef glm::vec<2, int64> Extent64;
typedef glm::vec<2, uint64> Extent64u;

// Vector are normally used with floating point types

typedef glm::vec<2, float> Vector2f;
typedef glm::vec<2, double> Vector2d;
typedef glm::vec<3, float> Vector3f;
typedef glm::vec<3, double> Vector3d;
typedef glm::vec<4, float> Vector4f;
typedef glm::vec<4, double> Vector4d;

typedef glm::vec<3, int32> Vector3i;
typedef glm::vec<3, uint32> uVector3i;
typedef glm::vec<4, int32> Vector4i;
typedef glm::vec<4, uint32> uVector4i;

// Matrices are always floating point types

typedef glm::mat<4, 4, float> Matrix4f;

// Not all platforms have wide characters as a defined length
// To keep it consistent, these macros will change depending on the platform

#define text(x) x
typedef const char* SChar;

// Gives the ability for an enum to be saved or loaded to/from streams
#define ENUM_OPERATORS(EnumClass, inType) \
	inline std::ostream& operator<<(std::ostream& outStream, const EnumClass& inEnumClass) { \
		outStream << static_cast<inType>(inEnumClass); \
		return outStream; \
	}\
	inline std::istream& operator>>(std::istream& inStream, EnumClass& inEnumClass) { \
		inType value; \
		inStream >> value; \
		inEnumClass = static_cast<EnumClass>(value); \
		return inStream; \
	}