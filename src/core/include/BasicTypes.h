#pragma once

#include "fmt/format.h"
#include <iostream>

typedef unsigned char uint8;
typedef signed char int8;
typedef unsigned short uint16;
typedef signed short int16;
typedef unsigned int uint32;
typedef signed int int32;
typedef unsigned long long uint64;
typedef signed long long int64;

template <typename TType>
struct TType2 {
	using Type = TType;
	Type x, y;
};

template <typename TType>
struct TType3 {
	using Type = TType;
	Type x, y, z;
};

template <typename TType>
struct TType4 {
	using Type = TType;
	Type x, y, z, w;
};

// Extent should always be used with integer types

typedef TType2<int32> Extent32;
typedef TType2<uint32> Extent32u;
typedef TType2<int64> Extent64;
typedef TType2<uint64> Extent64u;

// Vector should always be used with floating point types

typedef TType2<float> Vector2f;
typedef TType2<double> Vector2d;
typedef TType3<float> Vector3f;
typedef TType3<double> Vector3d;
typedef TType4<float> Vector4f;
typedef TType4<double> Vector4d;

// Not all platforms have wide characters as a defined length
// To keep it consistent, these macros will change depending on the platform

#define text(x) L##x
typedef const wchar_t* SChar;