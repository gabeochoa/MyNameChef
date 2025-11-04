
#ifdef BACKWARD
#include "backward/backward.hpp"
namespace backward {
backward::SignalHandling sh;
} // namespace backward
#endif

#include "game.h"
//
#include "argh.h"
#include "components/battle_anim_keys.h"
#include "components/side_effect_tracker.h"
#include "preload.h"
#include "seeded_rng.h"
#include "settings.h"
#include "shop.h"
#include "sound_systems.h"
#include "systems/AdvanceCourseSystem.h"
#include "systems/ApplyPairingsAndClashesSystem.h"
#include "systems/ApplyPendingCombatModsSystem.h"
#include "systems/AuditSystem.h"
#include "systems/BattleDebugSystem.h"
#include "systems/BattleEnterAnimationSystem.h"
#include "systems/BattleProcessorSystem.h"
#include "systems/BattleTeamLoaderSystem.h"
#include "systems/CleanupBattleEntities.h"
#include "systems/CleanupDishesEntities.h"
#include "systems/CleanupShopEntities.h"
#include "systems/ComputeCombatStatsSystem.h"
#include "systems/DropWhenNoLongerHeld.h"
#include "systems/EffectResolutionSystem.h"
#include "systems/GenerateDishesGallery.h"
#include "systems/HandleFreezeIconClick.h"
#include "systems/InitCombatState.h"
#include "systems/InitialShopFill.h"
#include "systems/LoadBattleResults.h"
#include "systems/MarkEntitiesWithShaders.h"
#include "systems/MarkIsHeldWhenHeld.h"
#include "systems/PostProcessingSystems.h"
#include "systems/ProcessBattleRewards.h"
#include "systems/RenderAnimations.h"
#include "systems/RenderBattleResults.h"
#include "systems/RenderBattleTeams.h"
#include "systems/RenderDebugWindowInfo.h"
#include "systems/RenderDishProgressBars.h"
#include "systems/RenderEntitiesByOrder.h"
#include "systems/RenderFPS.h"
#include "systems/RenderFreezeIcon.h"
#include "systems/RenderLetterboxBars.h"
#include "systems/RenderRenderTexture.h"
#include "systems/RenderSellSlot.h"
#include "systems/RenderSpritesByOrder.h"
#include "systems/RenderSpritesWithShaders.h"
#include "systems/RenderSystemHelpers.h"
#include "systems/RenderWalletHUD.h"
#include "systems/RenderZingBodyOverlay.h"
#include "systems/ReplayControllerSystem.h"
#include "systems/ResolveCombatTickSystem.h"
#include "systems/SimplifiedOnServeSystem.h"
#include "systems/StartCourseSystem.h"
#include "systems/TagShaderRender.h"
#include "systems/TestSystem.h"
#include "systems/TooltipSystem.h"
#include "systems/TriggerDispatchSystem.h"
#include "systems/UnifiedAnimationSystem.h"
#include "systems/UpdateRenderTexture.h"
#include "systems/UpdateSpriteTransform.h"
#include "ui/ui_systems.h"
#include <afterhours/src/plugins/animation.h>
#include <optional>

// TODO add honking

bool running = true;
// TODO move into library or somethign
raylib::RenderTexture2D mainRT;
raylib::RenderTexture2D screenRT;

// Global headless mode flag
bool render_backend::is_headless_mode = false;
// Step delay for non-headless test mode (milliseconds)
int render_backend::step_delay_ms = 500;
// Global audit strict mode flag
bool audit_strict = false;

using namespace afterhours;

void game(const std::optional<std::string> &run_test) {
  if (!render_backend::is_headless_mode) {
    mainRT = raylib::LoadRenderTexture(Settings::get().get_screen_width(),
                                       Settings::get().get_screen_height());
    screenRT = raylib::LoadRenderTexture(Settings::get().get_screen_width(),
                                         Settings::get().get_screen_height());
  }

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
    make_combat_manager(sophie);
    make_battle_processor_manager(sophie);

    // Initialize SeededRng singleton for deterministic randomness
    {
      auto &rng = SeededRng::get();
      rng.randomize_seed();
      log_info("Initialized SeededRng with seed {}", rng.seed);
    }

    if (audit_strict) {
      auto tracker = EntityHelper::get_singleton<SideEffectTracker>();
      if (tracker.get().has<SideEffectTracker>()) {
        tracker.get().get<SideEffectTracker>().enabled = true;
        log_info("AUDIT: Side effect tracking enabled");
      }
    }
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
    systems.register_update_system(std::make_unique<HandleFreezeIconClick>());
    systems.register_update_system(std::make_unique<MarkIsHeldWhenHeld>());
    systems.register_update_system(std::make_unique<DropWhenNoLongerHeld>());
    systems.register_update_system(std::make_unique<BattleTeamLoaderSystem>());
    systems.register_update_system(std::make_unique<BattleDebugSystem>());
    systems.register_update_system(std::make_unique<BattleProcessorSystem>());
    systems.register_update_system(
        std::make_unique<TriggerDispatchSystem>()); // Order events first
    systems.register_update_system(
        std::make_unique<EffectResolutionSystem>()); // Then process effects and
                                                     // clear
    systems.register_update_system(
        std::make_unique<ApplyPendingCombatModsSystem>());
    // Legacy battle systems - can be removed once BattleProcessor is working
    systems.register_update_system(std::make_unique<InitCombatState>());
    systems.register_update_system(
        std::make_unique<ApplyPairingsAndClashesSystem>());
    systems.register_update_system(std::make_unique<StartCourseSystem>());
    systems.register_update_system(
        std::make_unique<BattleEnterAnimationSystem>());
    // Compute after phase transitions to ensure current/base sync reflects
    // latest phase
    systems.register_update_system(
        std::make_unique<ComputeCombatStatsSystem>());
    systems.register_update_system(std::make_unique<SimplifiedOnServeSystem>());
    systems.register_update_system(std::make_unique<ResolveCombatTickSystem>());
    systems.register_update_system(std::make_unique<AdvanceCourseSystem>());
    systems.register_update_system(std::make_unique<ReplayControllerSystem>());
    systems.register_update_system(std::make_unique<AuditSystem>());
    systems.register_update_system(std::make_unique<CleanupBattleEntities>());
    systems.register_update_system(std::make_unique<CleanupShopEntities>());
    systems.register_update_system(std::make_unique<CleanupDishesEntities>());
    systems.register_update_system(std::make_unique<GenerateDishesGallery>());
    texture_manager::register_update_systems(systems);
    afterhours::animation::register_update_systems<
        afterhours::animation::CompositeKey>(systems);
    afterhours::animation::register_update_systems<BattleAnimKey>(systems);
    systems.register_update_system(
        std::make_unique<AnimationSchedulerSystem>());
    systems.register_update_system(std::make_unique<AnimationTimerSystem>());
    systems.register_update_system(
        std::make_unique<SlideInAnimationDriverSystem>());

    register_sound_systems(systems);
    register_ui_systems(systems);

    if (run_test.has_value()) {
      systems.register_update_system(std::make_unique<TestSystem>(*run_test));
    }

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
    if (!render_backend::is_headless_mode) {
      systems.register_render_system(std::make_unique<BeginWorldRender>());
      register_shop_render_systems(systems);

      {
        systems.register_render_system(std::make_unique<BeginCameraMode>());
        systems.register_render_system(
            std::make_unique<RenderEntitiesByOrder>());
        systems.register_render_system(std::make_unique<RenderBattleTeams>());
        systems.register_render_system(
            std::make_unique<RenderSpritesByOrder>());
        systems.register_render_system(
            std::make_unique<RenderSpritesWithShaders>());
        // Zing/Body overlays on top of sprites
        systems.register_render_system(
            std::make_unique<RenderZingBodyOverlay>());
        systems.register_render_system(std::make_unique<RenderAnimations>());
        systems.register_render_system(
            std::make_unique<RenderDishProgressBars>());
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
      systems.register_render_system(std::make_unique<RenderSellSlot>());
      systems.register_render_system(std::make_unique<RenderFreezeIcon>());
      systems.register_render_system(std::make_unique<RenderBattleResults>());
      // ReplayUISystem is now integrated into ScheduleMainMenuUI::battle_screen
      systems.register_render_system(std::make_unique<RenderTooltipSystem>());
      systems.register_render_system(std::make_unique<RenderFPS>());
      systems.register_render_system(std::make_unique<RenderDebugWindowInfo>());
      systems.register_render_system(std::make_unique<EndDrawing>());
      //
    }

    if (!render_backend::is_headless_mode) {
      while (running && !raylib::WindowShouldClose()) {
        systems.run(raylib::GetFrameTime());
      }
    } else {
      // Headless loop: run with fixed timestep; tests will exit the process
      while (running) {
        systems.run(1.0f / 60.0f);
      }
    }
  }
}

int main(int argc, char *argv[]) {

  // if nothing else ends up using this, we should move into preload.cpp
  argh::parser cmdl(argc, argv, argh::parser::PREFER_PARAM_FOR_UNREG_OPTION);

  int screenWidth, screenHeight;
  cmdl({"-w", "--width"}, 1280) >> screenWidth;
  cmdl({"-h", "--height"}, 720) >> screenHeight;

  std::optional<std::string> run_test = std::nullopt;
  std::string test_value;
  if (cmdl("--run-test") >> test_value) {
    if (!test_value.empty()) {
      run_test = test_value;
      log_info("TEST FLAG PARSED: {} - Test flag detected", test_value);
    }
  }

  // Parse headless mode flag
  bool headless_mode = false;
  if (cmdl["--headless"]) {
    headless_mode = true;
    render_backend::is_headless_mode = true;
    log_info("HEADLESS MODE: Enabled - Rendering will be skipped");
  }

  // Parse audit-strict mode flag
  if (cmdl["--audit-strict"]) {
    audit_strict = true;
    log_info("AUDIT STRICT: Enabled - Side effect violations will be logged");
  }

  // Parse step delay flag (only used in non-headless mode)
  int step_delay = 500; // Default 500ms
  cmdl({"--step-delay"}, 500) >> step_delay;
  render_backend::step_delay_ms = step_delay;
  if (!headless_mode && step_delay > 0) {
    log_info("STEP DELAY: {}ms between test steps", step_delay);
  }

  // Load savefile first
  Settings::get().load_save_file(screenWidth, screenHeight);

  Preload::get() //
      .init("template", headless_mode)
      .make_singleton();
  Settings::get().refresh_settings();

  // if (cmdl[{"-i", "--show-intro"}]) {
  //   intro();
  // }

  game(run_test);

  Settings::get().write_save_file();

  return 0;
}
