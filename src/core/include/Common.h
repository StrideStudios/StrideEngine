#pragma once

#include <iostream>
#include <cassert>
#include "fmt/format.h"
#include "BasicTypes.h"

/*
 * To keep it simple yet readable these are some naming conventions:
 *
 * Classes (Should not be data only): CClassName
 * Interfaces (abstract Classes with pure virual functions) IInterface
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
 * Variables should always be below Functions
 * The order of Functions and Variables should be as follows:
 * constexpr/static
 * const
 * non-const
 *
 * Functions are in camel case, ex: startMainFunction
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

constexpr static uint8 gFrameOverlap = 2;

//
// Common but hard to write forward includes
//

typedef std::shared_ptr<struct SImage_T> SImage;
typedef std::shared_ptr<struct SBuffer_T> SBuffer;
typedef std::shared_ptr<struct SMeshBuffers_T> SMeshBuffers;

//
// Ugly c++ replacement Macros
//

#define no_discard [[nodiscard]]
#define force_inline __forceinline

//
// Text formatting/Assertion macros
//

#define fmts(x, ...) fmt::format(x, __VA_ARGS__)
#define msgs(x, ...) std::cout << fmts(x, __VA_ARGS__) << std::endl
#define errs(x, ...) throw std::runtime_error(fmts(x, __VA_ARGS__))
#define asts(cond, x, ...) if (!(cond)) errs(x, __VA_ARGS__)

#define astsNoEntry() errs("Code block should not have been called!")

// Use this to ensure that an area of code is only called once
#define astsOnce(name) \
	{ static bool beenHere##name = false; \
	asts(!beenHere##name, "Code block {} called more than once.", #name); \
	beenHere##name = true; }