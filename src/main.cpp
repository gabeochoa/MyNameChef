
#ifdef BACKWARD
#include "backward/backward.hpp"
namespace backward {
backward::SignalHandling sh;
} // namespace backward
#endif

#include "game.h"
//
#include "argh.h"
#include "game_state_manager.h"
#include "preload.h"
#include "settings.h"
#include "shop.h"
#include "sound_systems.h"
#include "systems/begin_post_processing_shader.h"
#include "systems/mark_entities_with_shaders.h"
#include "systems/post_processing_systems.h"
#include "systems/render_debug_window_info.h"
#include "systems/render_entities.h"
#include "systems/render_fps.h"
#include "systems/render_letterbox_bars.h"
#include "systems/render_sprites_with_shaders.h"
#include "systems/render_system_helpers.h"
#include "systems/tag_shader_render.h"
#include "systems/update_render_texture.h"
#include "systems/update_sprite_transform.h"
#include "ui/ui_systems.h"
#include <afterhours/src/plugins/animation.h>

// TODO add honking

bool running = true;
// TODO move into library or somethign
raylib::RenderTexture2D mainRT;
raylib::RenderTexture2D screenRT;

using namespace afterhours;

void game() {
  mainRT = raylib::LoadRenderTexture(Settings::get().get_screen_width(),
                                     Settings::get().get_screen_height());
  screenRT = raylib::LoadRenderTexture(Settings::get().get_screen_width(),
                                       Settings::get().get_screen_height());

  SystemManager systems;

  // debug systems
  {
    window_manager::enforce_singletons(systems);
    ui::enforce_singletons<InputAction>(systems);
    input::enforce_singletons(systems);
    texture_manager::enforce_singletons(systems);
    enforce_ui_singletons(systems);
  }

  // external plugins
  {
    input::register_update_systems(systems);
    window_manager::register_update_systems(systems);
  }

  // Fixed update
  {}

  // normal update
  {
    bool create_startup = true;
    systems.register_update_system([&](float) {
      if (create_startup) {
        create_startup = false;
        make_shop_manager();
      }
    });

    systems.register_update_system(std::make_unique<UpdateSpriteTransform>());
    systems.register_update_system(std::make_unique<UpdateShaderValues>());
    systems.register_update_system(std::make_unique<MarkEntitiesWithShaders>());
    texture_manager::register_update_systems(systems);

    register_sound_systems(systems);
    register_ui_systems(systems);

    systems.register_update_system(std::make_unique<UpdateRenderTexture>());
    systems.register_update_system(std::make_unique<MarkEntitiesWithShaders>());

    // renders
    {
      systems.register_render_system(std::make_unique<BeginWorldRender>());

      {
        systems.register_render_system(std::make_unique<BeginCameraMode>());
        systems.register_render_system(std::make_unique<RenderEntities>());
        texture_manager::register_render_systems(systems);
        systems.register_render_system(
            std::make_unique<RenderSpritesWithShaders>());
        systems.register_render_system(std::make_unique<EndCameraMode>());
        // (UI moved to pass 2 so it is after tag shader)
      }
      systems.register_render_system(std::make_unique<EndWorldRender>());
      // pass 2: render mainRT with tag shader into screenRT, then draw UI into
      // screenRT
      systems.register_render_system(std::make_unique<BeginTagShaderRender>());
      ui::register_render_systems<InputAction>(
          systems, InputAction::ToggleUILayoutDebug);
      systems.register_render_system(std::make_unique<EndTagShaderRender>());
      // pass 3: draw to screen with base post-processing shader
      systems.register_render_system(
          std::make_unique<BeginPostProcessingRender>());
      systems.register_render_system(
          std::make_unique<SetupPostProcessingShader>());

      systems.register_render_system(std::make_unique<RenderScreenToWindow>());
      systems.register_render_system(
          std::make_unique<EndPostProcessingShader>());
      systems.register_render_system(std::make_unique<RenderLetterboxBars>());
      systems.register_render_system(std::make_unique<RenderFPS>());
      systems.register_render_system(std::make_unique<RenderDebugWindowInfo>());

      systems.register_render_system(std::make_unique<EndDrawing>());
      //
    }

    while (running && !raylib::WindowShouldClose()) {
      systems.run(raylib::GetFrameTime());
    }

    std::cout << "Num entities: " << EntityHelper::get_entities().size()
              << std::endl;
  }
}

int main(int argc, char *argv[]) {

  // if nothing else ends up using this, we should move into preload.cpp
  argh::parser cmdl(argc, argv, argh::parser::PREFER_PARAM_FOR_UNREG_OPTION);

  int screenWidth, screenHeight;
  cmdl({"-w", "--width"}, 1280) >> screenWidth;
  cmdl({"-h", "--height"}, 720) >> screenHeight;

  // Load savefile first
  Settings::get().load_save_file(screenWidth, screenHeight);

  Preload::get() //
      .init("template")
      .make_singleton();
  Settings::get().refresh_settings();

  // if (cmdl[{"-i", "--show-intro"}]) {
  //   intro();
  // }

  game();

  Settings::get().write_save_file();

  return 0;
}
