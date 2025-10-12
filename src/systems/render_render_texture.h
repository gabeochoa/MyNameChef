#include "letterbox_layout.h"

struct RenderRenderTexture : System<window_manager::ProvidesCurrentResolution> {
  virtual ~RenderRenderTexture() {}
  virtual void for_each_with(const Entity &,
                             const window_manager::ProvidesCurrentResolution &,
                             float) const override {
    const int window_w = raylib::GetScreenWidth();
    const int window_h = raylib::GetScreenHeight();
    const int content_w = mainRT.texture.width;
    const int content_h = mainRT.texture.height;

    const LetterboxLayout layout =
        compute_letterbox_layout(window_w, window_h, content_w, content_h);

    const raylib::Rectangle src{0.0f, 0.0f, (float)mainRT.texture.width,
                                -(float)mainRT.texture.height};
    raylib::DrawTexturePro(mainRT.texture, src, layout.dst, {0.0f, 0.0f}, 0.0f,
                           raylib::WHITE);
  }
};
