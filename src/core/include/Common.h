#pragma once

#include <cassert>

/*
 * To keep it simple yet readable these are some naming conventions:
 *
 * Classes (Should not be data only): CClassName
 * Structs (Should be data only with only basic functions): SStructName
 * Enums (Should always be enum classes): EEnumName
 * Enums should also always try to use the lowest integer type it can
 * Enum flags should use the 1 << 0 and 1 << 1 format for readability
 *
 * Member variable: mVariable (Private: m_Variable)
 * Static variable: sVariable (Private: s_Variable)
 * Global variable: gVariable
 * Local variable: variable
 *
 * Classes and Structs should not include more than one private: protected: and public: identifier
 * Variables should always be below Functions
 * The order of Functions and Variables should be as follows:
 * constexpr/static
 * const
 * non-const
 *
 * Functions are in camel case, ex: startMainFunction
 *
 * multi line variable initialization should have another tab indent:
 * auto vkbInstance = builder.set_app_name("Stride Engine")
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

//
// Ugly c++ replacement Macros
//

#define no_discard [[nodiscard]]
#define force_inline __forceinline

typedef unsigned char uint8;
typedef signed char int8;
typedef unsigned short uint16;
typedef signed short int16;
typedef unsigned int uint32;
typedef signed int int32;
typedef unsigned long long uint64;
typedef signed long long int64;

typedef float float32;
typedef double double32;