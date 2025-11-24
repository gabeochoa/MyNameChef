
#pragma once

// Include external.h first to ensure AFTER_HOURS_USE_RAYLIB and type macros
// are defined before any afterhours headers
#include "external.h"

#include "rl.h"

// Owned by main.cpp
extern bool running;
// TODO move into library or somethign
extern raylib::RenderTexture2D mainRT;
// second render texture for compositing tag shader and UI
extern raylib::RenderTexture2D screenRT;
