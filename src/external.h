
#pragma once

// Include raylib types first
#include "std_include.h"

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#pragma clang diagnostic ignored "-Wdeprecated-volatile"
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wall"
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#pragma GCC diagnostic ignored "-Wpedantic"
#pragma GCC diagnostic ignored "-Wfloat-conversion"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wdangling-reference"
#endif

namespace raylib {
#if defined(__has_include)
#if __has_include(<raylib.h>)
#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>
#elif __has_include("raylib/raylib.h")
#include "raylib/raylib.h"
#include "raylib/raymath.h"
#include "raylib/rlgl.h"
#else
#error "raylib headers not found"
#endif
#else
#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>
#endif
} // namespace raylib

// Define AFTER_HOURS_USE_RAYLIB and type macros BEFORE any afterhours includes
// This ensures they're set globally, matching pharmasea's approach in pch.hpp
// These must be defined before any afterhours headers are included
#define AFTER_HOURS_USE_RAYLIB
#undef RectangleType
#undef Vector2Type
#undef TextureType
#undef FontType
#define RectangleType raylib::Rectangle
#define Vector2Type raylib::Vector2
#define TextureType raylib::Texture2D
#define FontType raylib::Font
#define AFTER_HOURS_RECTANGLE_TYPE_DEFINED
#define AFTER_HOURS_VECTOR2_TYPE_DEFINED

#ifdef __clang__
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

