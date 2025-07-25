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

template <typename TType>
requires std::is_arithmetic_v<TType>
struct TType2 {
	using Type = TType;
	Type x, y;

	TType2() = default;

	TType2(Type x, Type y):
	x(x), y(y) {}

	TType2(glm::vec2 inVector):
	x((Type)inVector.x),
	y((Type)inVector.y) {}

	operator glm::vec2() const {
		return {x, y};
	}

	TType2 operator+(const TType2& otherType) {
		return {x + otherType.x, y + otherType.y};
	}

	TType2 operator-(const TType2& otherType) {
		return {x - otherType.x, y - otherType.y};
	}

	TType2 operator*(const TType2& otherType) {
		return {x * otherType.x, y * otherType.y};
	}

	TType2 operator/(const TType2& otherType) {
		return {x / otherType.x, y / otherType.y};
	}
};

template <typename TType>
requires std::is_arithmetic_v<TType>
struct TType3 {
	using Type = TType;
	Type x, y, z;

	TType3() = default;

	TType3(Type x, Type y, Type z):
	x(x), y(y), z(z) {}

	TType3(glm::vec3 inVector):
	x((Type)inVector.x),
	y((Type)inVector.y),
	z((Type)inVector.z) {}

	operator glm::vec3() const {
		return {x, y, z};
	}

	TType3 operator+(const TType3& b) const {
		return {x + b.x, y + b.y, z + b.z};
	}

	TType3 operator-(const TType3& b) const {
		return {x - b.x, y - b.y, z - b.z};
	}

	TType3 operator*(const TType3& b) const {
		return {x * b.x, y * b.y, z * b.z};
	}

	TType3 operator/(const TType3& b) const {
		return {x / b.x, y / b.y, z / b.z};
	}
};

template <typename TType>
requires std::is_arithmetic_v<TType>
struct TType4 {
	using Type = TType;
	Type x, y, z, w;

	TType4() = default;

	TType4(Type x, Type y, Type z, Type w):
	x(x), y(y), z(z), w(w) {}

	TType4(glm::vec4 inVector):
	x((Type)inVector.x),
	y((Type)inVector.y),
	z((Type)inVector.z),
	w((Type)inVector.w) {}

	operator glm::vec4() const {
		return {x, y, z, w};
	}

	TType4 operator+(const TType4& b) const {
		return {x + b.x, y + b.y, z + b.z, w + b.w};
	}

	TType4 operator-(const TType4& b) const {
		return {x - b.x, y - b.y, z - b.z, w - b.w};
	}

	TType4 operator*(const TType4& b) const {
		return {x * b.x, y * b.y, z * b.z, w * b.w};
	}

	TType4 operator*(const float& f) const {
		return {x * f, y * f, z * f, w * f};
	}

	TType4 operator/(const TType4& b) const {
		return {x / b.x, y / b.y, z / b.z, w / b.w};
	}
};


template <typename TType>
requires std::is_arithmetic_v<TType>
struct TType4x4 {
	using Type = TType;
	TType4<Type> x, y, z, w;

	TType4x4() = default;

	TType4x4(Type x, Type y, Type z, Type w):
	x(x), y(y), z(z), w(w) {}

	TType4x4(glm::mat4 inMatrix) {
		x = inMatrix[0];
		y = inMatrix[1];
		z = inMatrix[2];
		w = inMatrix[3];
	}

	operator glm::mat4() const {
		return {x, y, z, w};
	}

	/*TType4x4 operator+(const TType4x4& b) const {
		return {x + b.x, y + b.y, z + b.z, w + b.w};
	}

	TType4x4 operator-(const TType4x4& b) const {
		return {x - b.x, y - b.y, z - b.z, w - b.w};
	}*/

	TType4x4 operator*(const TType4x4& b) const {
		const TType4<Type> SrcA0 = x;
		const TType4<Type> SrcA1 = y;
		const TType4<Type> SrcA2 = z;
		const TType4<Type> SrcA3 = w;

		const TType4<Type> SrcB0 = b.x;
		const TType4<Type> SrcB1 = b.y;
		const TType4<Type> SrcB2 = b.z;
		const TType4<Type> SrcB3 = b.w;

		TType4x4 Result;
		Result.x = SrcA0 * SrcB0.x + SrcA1 * SrcB0.y + SrcA2 * SrcB0.z + SrcA3 * SrcB0.w;
		Result.y = SrcA0 * SrcB1.x + SrcA1 * SrcB1.y + SrcA2 * SrcB1.z + SrcA3 * SrcB1.w;
		Result.z = SrcA0 * SrcB2.x + SrcA1 * SrcB2.y + SrcA2 * SrcB2.z + SrcA3 * SrcB2.w;
		Result.w = SrcA0 * SrcB3.x + SrcA1 * SrcB3.y + SrcA2 * SrcB3.z + SrcA3 * SrcB3.w;
		return Result;
	}

	/*TType4x4 operator/(const TType4x4& b) const {
		return {x / b.x, y / b.y, z / b.z, w / b.w};
	}*/ //TODO: matrix operations tend to work differently

	// The identity matrix
	constexpr static TType4x4 Identity() {
		return {
			.x = {1, 0, 0, 0},
			.y = {0, 1, 0, 0},
			.z = {0, 0, 1, 0},
			.w = {0, 0, 0, 1}
		};
	}
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

// Matrices are always floating point types

typedef TType4x4<float> Matrix4f;

// Not all platforms have wide characters as a defined length
// To keep it consistent, these macros will change depending on the platform

#define text(x) x
typedef const char* SChar;