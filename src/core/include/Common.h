#pragma once

#include <iostream>
#include <cassert>
#include "fmt/format.h"
#include "BasicTypes.h"

/*
 * To keep it simple yet readable these are some naming conventions:
 *
 * Classes (Should not be data only): CClassName
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

//
// Text formatting/Assertion macros
//

#define fmt(x, ...) fmt::format(x, __VA_ARGS__)
#define msg(x, ...) std::cout << fmt(x, __VA_ARGS__) << std::endl
#define err(x, ...) throw std::runtime_error(fmt(x, __VA_ARGS__))
#define ast(cond, x, ...) if (!(cond)) err(x, __VA_ARGS__)

#define astNoEntry() err("Code block should not have been called!")

// Use this to ensure that an area of code is only called once
#define astOnce(name) \
	{ static bool beenHere##name = false; \
	ast(!beenHere##name, "Code block {} called more than once.", #name); \
	beenHere##name = true; }