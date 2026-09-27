#pragma once

#include "../../render_backend.h"
#include "../../rl.h"
#include "../../settings.h"
#include "../../texture_library.h"
#include "../test_app.h"
#include "../test_macros.h"
#include <afterhours/ah.h>

// Render textures created by game() before the test loop starts.
extern raylib::RenderTexture2D mainRT;
extern raylib::RenderTexture2D screenRT;

TEST(validate_headless_render) {
  app.launch_game();
  app.wait_for_frames(5);

  if (render_backend::is_offscreen_render_mode) {
    // --headless-render: a hidden window provides a real GL context, the
    // render targets and textures exist, and frames really draw - while
    // the loop keeps headless (fixed-dt) semantics.
    app.expect_true(render_backend::is_headless_mode,
                    "offscreen render keeps headless loop semantics");
    app.expect_true(render_backend::should_render(),
                    "offscreen render enables rendering");
    app.expect_true(raylib::IsWindowReady(),
                    "offscreen render has a (hidden) window");
    app.expect_eq(raylib::GetScreenWidth(),
                  Settings::get().get_screen_width(),
                  "offscreen window has the configured width");
    app.expect_true(mainRT.id > 0 && mainRT.texture.id > 0,
                    "main render texture was created");
    app.expect_true(screenRT.id > 0 && screenRT.texture.id > 0,
                    "screen render texture was created");
    app.expect_true(TextureLibrary::get().get("dollar_sign").id > 0,
                    "spritesheet-era textures are really loaded");
  } else if (render_backend::is_headless_mode) {
    // Plain --headless: no window, no GL, rendering fully skipped.
    app.expect_false(render_backend::should_render(),
                     "plain headless skips rendering");
    app.expect_false(raylib::IsWindowReady(),
                     "plain headless has no window");
    app.expect_true(mainRT.id == 0,
                    "plain headless creates no render texture");
  } else {
    // Visible mode: window and rendering as before.
    app.expect_true(render_backend::should_render(),
                    "visible mode renders");
    app.expect_true(raylib::IsWindowReady(), "visible mode has a window");
    app.expect_true(mainRT.id > 0, "visible mode has a render texture");
  }
}
