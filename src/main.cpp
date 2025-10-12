
#ifdef BACKWARD
#include "backward/backward.hpp"
namespace backward {
backward::SignalHandling sh;
} // namespace backward
#endif

#include "game.h"
//
#include "argh.h"
#include "preload.h"
#include "settings.h"
#include "shop.h"
#include "sound_systems.h"
#include "systems/BattleAnimations.h"
#include "systems/BattleDebugSystem.h"
#include "systems/BattleTeamLoaderSystem.h"
#include "systems/CleanupBattleEntities.h"
#include "systems/CleanupShopEntities.h"
#include "systems/DropWhenNoLongerHeld.h"
#include "systems/InitialShopFill.h"
#include "systems/JudgingSystems.h"
#include "systems/LoadBattleResults.h"
#include "systems/MarkEntitiesWithShaders.h"
#include "systems/MarkIsHeldWhenHeld.h"
#include "systems/PostProcessingSystems.h"
#include "systems/ProcessBattleRewards.h"
#include "systems/RenderBattleResults.h"
#include "systems/RenderEntitiesByOrder.h"
#include "systems/RenderSpritesByOrder.h"
#include "systems/RenderBattleTeams.h"
#include "systems/RenderDebugWindowInfo.h"
#include "systems/RenderEntities.h"
#include "systems/RenderFPS.h"
#include "systems/RenderJudges.h"
#include "systems/RenderLetterboxBars.h"
#include "systems/RenderRenderTexture.h"
#include "systems/RenderScoringBar.h"
#include "systems/RenderSpritesWithShaders.h"
#include "systems/RenderSystemHelpers.h"
#include "systems/RenderWalletHUD.h"
#include "systems/TagShaderRender.h"
#include "systems/TooltipSystem.h"
#include "systems/UpdateRenderTexture.h"
#include "systems/UpdateSpriteTransform.h"
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
  auto &sophie = EntityHelper::createEntity();

  // singleton systems
  {
    window_manager::enforce_singletons(systems);
    ui::enforce_singletons<InputAction>(systems);
    input::enforce_singletons(systems);
    texture_manager::enforce_singletons(systems);
    enforce_ui_singletons(systems);
    make_shop_manager(sophie);
    make_judging_manager(sophie);
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
      }
    });

    systems.register_update_system(std::make_unique<UpdateSpriteTransform>());
    systems.register_update_system(std::make_unique<UpdateShaderValues>());
    systems.register_update_system(std::make_unique<MarkEntitiesWithShaders>());
    systems.register_update_system(std::make_unique<MarkIsHeldWhenHeld>());
    systems.register_update_system(std::make_unique<DropWhenNoLongerHeld>());
    systems.register_update_system(std::make_unique<BattleTeamLoaderSystem>());
    systems.register_update_system(std::make_unique<BattleDebugSystem>());
    systems.register_update_system(std::make_unique<InitJudgingState>());
    systems.register_update_system(std::make_unique<AdvanceJudging>());
    systems.register_update_system(std::make_unique<CleanupBattleEntities>());
    systems.register_update_system(std::make_unique<CleanupShopEntities>());
    texture_manager::register_update_systems(systems);
    afterhours::animation::register_update_systems<
        afterhours::animation::CompositeKey>(systems);
    afterhours::animation::register_update_systems<BattleAnimKey>(systems);
    systems.register_update_system(std::make_unique<TriggerBattleSlideIn>());

    register_sound_systems(systems);
    register_ui_systems(systems);
    // Ensure results are loaded in the same frame UI switches to Results
    systems.register_update_system(std::make_unique<LoadBattleResults>());
    // Process battle rewards and refill store after results are loaded
    systems.register_update_system(std::make_unique<ProcessBattleRewards>());
    // Fill shop on first game start
    systems.register_update_system(std::make_unique<InitialShopFill>());
    register_shop_update_systems(systems);

    systems.register_update_system(std::make_unique<UpdateRenderTexture>());
    systems.register_update_system(std::make_unique<MarkEntitiesWithShaders>());

    // renders
    {
      systems.register_render_system(std::make_unique<BeginWorldRender>());
      register_shop_render_systems(systems);

      {
        systems.register_render_system(std::make_unique<BeginCameraMode>());
        systems.register_render_system(std::make_unique<RenderEntitiesByOrder>());
        systems.register_render_system(std::make_unique<RenderBattleTeams>());
        systems.register_render_system(std::make_unique<RenderJudges>());
        systems.register_render_system(std::make_unique<RenderSpritesByOrder>());
        systems.register_render_system(
            std::make_unique<RenderSpritesWithShaders>());
        systems.register_render_system(std::make_unique<RenderWalletHUD>());
        systems.register_render_system(std::make_unique<EndCameraMode>());
        // (UI moved to pass 2 so it is after tag shader)
      }
      systems.register_render_system(std::make_unique<EndWorldRender>());

      // Draw mainRT to screen
      systems.register_render_system(std::make_unique<RenderRenderTexture>());

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
      systems.register_render_system(std::make_unique<RenderBattleResults>());
      systems.register_render_system(std::make_unique<RenderScoringBar>());
      systems.register_render_system(std::make_unique<RenderTooltipSystem>());
      systems.register_render_system(std::make_unique<RenderFPS>());
      systems.register_render_system(std::make_unique<RenderDebugWindowInfo>());
      systems.register_render_system(std::make_unique<EndDrawing>());
      //
    }

    while (running && !raylib::WindowShouldClose()) {
      systems.run(raylib::GetFrameTime());
    }
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
