#pragma once

#include <iostream>
#include "fmt/format.h"
#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>

//TODO: why this?
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/quaternion.hpp>

#define EXPORT __declspec(dllexport)
#define IMPORT __declspec(dllimport)

/*
 * To keep it simple yet readable these are some naming conventions:
 *
 * Classes (Should not be data only): CClassName
 * Interfaces (abstract Classes with pure virtual functions) IInterface
 * Structs (Should be data only with only basic functions): SStructName
 * Enums (Should always be enum classes): EEnumName enumerators should always be ALL_CAPS
 * Enums should also always try to use the lowest integer type it can
 * Enum flags should use the 1 << 0 and 1 << 1 format for readability
 *
 * The only exception for the names are nested classes/structs/enums
 *
 * Templated classes and structs: TName
 *
 * Member variable: mVariable (Private: m_Variable)
 * Static variable: sVariable (Private: s_Variable)
 * Global variable: gVariable
 * Local variable: variable
 *
 * Classes and Structs should not include more than one private: protected: and public: identifier
 * The order of Functions and Variables should be as follows:
 * constexpr/static
 * const
 * non-const
 *
 * Functions are in camel case, ex: startMainFunction
 * (typedef functions should start with an F and be uppercase, like FFunction)
 *
 * multi line variable initialization should have another tab indent:
 * auto vkbInstance = builder.set_app_name(gEngineName)
 *     .build();
 *
 * Macros: MACRO_NAME (exceptions for replacing ugly c++ code like below)
 * Multi-line Macros should always have their line identifier one space after the line end:
 * MACRO_NAME(x) \
 * print(x)
 *
 * Comments that are a single line can use //, other should use /*
 * Category Comments should have thee lines of // like below
 */

inline size_t getHash(const char c) {
	return c;
}

// Simple hashing, could potentially have repeats, but it is unlikely
// djb2 http://www.cse.yorku.ca/~oz/hash.html
inline size_t getHash(const std::string& string) {
	size_t Hash = 0;
	const unsigned char* str = (unsigned char*)string.c_str();

	int c;
	while ((c = *str++))
		Hash = ((Hash << 5) + Hash) + c;

	return Hash;
}

struct IDestroyable {
	virtual ~IDestroyable() = default;

	virtual void destroy() {}
};

template <typename... TArgs>
struct TInitializable {
	virtual void init(TArgs... args) {}
};

typedef TInitializable<> IInitializable;

template <bool TDefaultDirty = false>
struct TDirtyable {
	virtual ~TDirtyable() = default;

	bool isDirty() const { return m_Dirty; }

	void setDirty() {
		m_Dirty = true;
		onDirty();
	}

	virtual void onDirty() {}

	virtual void onClean() {}

public:

	void clean() {
		m_Dirty = false;
		onClean();
	}

private:

	bool m_Dirty = TDefaultDirty;
};

typedef unsigned char uint8;
typedef signed char int8;
typedef unsigned short uint16;
typedef signed short int16;
typedef unsigned int uint32;
typedef signed int int32;
typedef unsigned long long uint64;
typedef signed long long int64;

typedef union {
	float f;
	uint32 i;
} floatCast;

// Floating point data but half the size in memory
struct half {

	uint16 data = 0;

	half() = default;

	// Some very scary and advanced math that i dont understand
	// thanks to ProjectPhysX on https://stackoverflow.com/questions/1659440/32-bit-to-16-bit-floating-point-conversion
	half(const float f) {
		floatCast cast = {.f = f};

		const uint32 b = cast.i+0x00001000;
		const uint32 e = (b&0x7F800000)>>23;
		const uint32 m = b&0x007FFFFF;
		data = (b&0x80000000)>>16 | (e>112)*((((e-112)<<10)&0x7C00)|m>>13) | ((e<113)&(e>101))*((((0x007FF000+m)>>(125-e))+1)>>1) | (e>143)*0x7FFF;
	}

	operator float() const {
		const uint32 e = (data&0x7C00)>>10;
		const uint32 m = (data&0x03FF)<<13;
		floatCast cast = {.f = (float)m};
		const uint32 v = cast.i>>23;
		floatCast cast2 = {.i = (data&0x8000)<<16 | (e!=0)*((e+112)<<23|m) | ((e==0)&(m!=0))*((v-37)<<23|((m<<(150-v))&0x007FE000))};
		return cast2.f;
	}
};

#define PI 3.14159265359
#define DOUBLE_PI 6.28318530718

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

// Vector half types

typedef glm::vec<2, half> Vector2h;
typedef glm::vec<3, half> Vector3h;
typedef glm::vec<4, half> Vector4h;

// Represent an 8Bit Color
typedef glm::vec<3, uint8> Color3;
typedef glm::vec<4, uint8> Color4;

typedef glm::vec<3, int32> Vector3i;
typedef glm::vec<3, uint32> uVector3i;
typedef glm::vec<4, int32> Vector4i;
typedef glm::vec<4, uint32> uVector4i;

// Matrices are always floating point types

typedef glm::mat<4, 4, float> Matrix4f;
typedef glm::mat<4, 4, double> Matrix4d;

// Transforms combine position, rotation and scale.
struct Transform3f {
	Vector3f getPosition() const { return m_Position; }
	Vector3f getRotation() const { return m_Rotation; }
	Vector3f getScale() const { return m_Scale; }

	void setPosition(const Vector3f inPosition) {
		m_Position = inPosition;
		createMatrix();
	}
	void setRotation(const Vector3f inRotation) {
		m_Rotation = inRotation;
		createMatrix();
	}
	void setScale(const Vector3f inScale) {
		m_Scale = inScale;
		createMatrix();
	}

	Matrix4f toMatrix() const {
		return m_Transform;
	}

private:

	void createMatrix() {
		Matrix4f mat = glm::translate(Matrix4f(1.0), m_Position);
		mat = glm::scale(mat, m_Scale);
		mat *= glm::mat4_cast(glm::qua(m_Rotation / 180.f * (float)PI));
		m_Transform = mat;
	}

	Matrix4f m_Transform{1.f};

	Vector3f m_Position{0.f};
	Vector3f m_Rotation{0.f};
	Vector3f m_Scale{1.f};
};

// 2d transform (in screen coordinates)
struct Transform2f {
	Vector2f getOrigin() const { return m_Origin; }
	Vector2f getPosition() const { return m_Position; }
	float getRotation() const { return m_Rotation; }
	Vector2f getScale() const { return m_Scale; }

	void setOrigin(const Vector2f inOrigin) {
		m_Origin = inOrigin;
		createMatrix();
	}
	void setPosition(const Vector2f inPosition) {
		m_Position = inPosition;
		createMatrix();
	}
	void setRotation(const float inRotation) {
		m_Rotation = inRotation;
		createMatrix();
	}
	void setScale(const Vector2f inScale) {
		m_Scale = inScale;
		createMatrix();
	}

	Matrix4f toMatrix() const {
		return m_Transform;
	}

private:

	void createMatrix() {
		Matrix4f mat = glm::translate(Matrix4f(1.0), Vector3f(m_Position - m_Origin * m_Scale, 0.f));
		mat = glm::scale(mat, Vector3f(m_Scale, 1.f));
		mat *= glm::mat4_cast(glm::qua(Vector3f(0.f, 0.f, m_Rotation / 180.f * (float)PI)));
		m_Transform = mat;
	}

	Matrix4f m_Transform{1.f};

	Vector2f m_Origin{0.f};
	Vector2f m_Position{0.f};
	float m_Rotation = 0.f;
	Vector2f m_Scale{1.f};
};

// Not all platforms have wide characters as a defined length
// To keep it consistent, these macros will change depending on the platform

#define text(x) x
typedef const char* SChar;

constexpr static auto gEngineName = text("Stride Engine");

// Gives the enum some convenience operators
#define ENUM_OPERATORS(EnumName, inType) \
	inline E##EnumName to##EnumName(const inType& inValue) { return static_cast<E##EnumName>(inValue); } \
	inline inType valueOf(const E##EnumName& inValue) { return static_cast<inType>(inValue); } \
	inline bool operator==(const E##EnumName& lhs, const inType& rhs) { \
		return static_cast<inType>(lhs) == rhs; \
	}

#define ENUM_TO_STRING(EnumName, inType, ...) \
	constexpr static const char* EnumName##Map[] = {__VA_ARGS__}; \
	inline const char* get##EnumName##ToString(E##EnumName inValue) { \
		return EnumName##Map[(inType)inValue]; \
	}

//
// Ugly c++ replacement Macros
//

#define no_discard [[nodiscard]]
#define force_inline __forceinline

//
// Text formatting/Assertion macros
//

#define fmts(x, ...) fmt::format(x, __VA_ARGS__)
#define toText(x) fmts("{}", x)
#define msgs(x, ...) std::cout << fmts(x, __VA_ARGS__) << std::endl
#define errs(x, ...) msgs(x, __VA_ARGS__); throw std::runtime_error(fmts(x, __VA_ARGS__))
#define asts(cond, x, ...) if (!(cond)) { errs(x, __VA_ARGS__); }

#define astsNoEntry() errs("Code block should not have been called!")

// Use this to ensure that an area of code is only called once
#define astsOnce(name) \
	{ static bool beenHere##name = false; \
	asts(!beenHere##name, "Code block {} called more than once.", #name); \
	beenHere##name = true; }

#define CONCAT(x, y) _CONCAT(x,y)

#define MY_MACRO MY_MACRO_COUNTED(__COUNTER__)

#define MY_MACRO_COUNTED(counter) counter + counter

#define UNIQUE_VAR(x) CONCAT(x, __COUNTER__)

// Used to run code statically, returns int similar to main
#define STATIC_BLOCK_(id, ...) \
	inline static int CONCAT(__static_block, id) = [] { \
		__VA_ARGS__ \
		return 0; \
	}();

#define STATIC_BLOCK(...) STATIC_BLOCK_(__COUNTER__, __VA_ARGS__)

// Used to run code statically, returns int similar to main
#define STATIC_C_BLOCK_(id, ...) \
	struct CONCAT(__static_c_block, id) { \
		CONCAT(__static_c_block, id)() { __VA_ARGS__ } \
	}; \
	inline static CONCAT(__static_c_block, id) CONCAT(__block, id){};

#define STATIC_C_BLOCK(...) STATIC_C_BLOCK_(__COUNTER__, __VA_ARGS__)
