#pragma once

#include "../game.h"
#include "../render_backend.h"
#include "../query.h"
#include <afterhours/ah.h>

struct UpdateRenderTexture : System<> {

  window_manager::Resolution resolution;

  virtual ~UpdateRenderTexture() {}

  virtual bool should_run(float) override {
    return !render_backend::is_headless_mode;
  }

  void once(float) {
    const window_manager::ProvidesCurrentResolution *pcr =
        EntityHelper::get_singleton_cmp<
            window_manager::ProvidesCurrentResolution>();
    if (pcr->current_resolution != resolution) {
      resolution = pcr->current_resolution;
      raylib::UnloadRenderTexture(mainRT);
      mainRT = raylib::LoadRenderTexture(resolution.width, resolution.height);
      // keep the second render texture in sync
      raylib::UnloadRenderTexture(screenRT);
      screenRT = raylib::LoadRenderTexture(resolution.width, resolution.height);
    }
  }
};
