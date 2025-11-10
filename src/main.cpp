
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
#include "components/user_id.h"
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
#include "systems/NetworkSystem.h"
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
#include "systems/RenderToastSystem.h"
#include "systems/RenderWalletHUD.h"
#include "systems/RenderZingBodyOverlay.h"
#include "systems/ReplayControllerSystem.h"
#include "systems/ResolveCombatTickSystem.h"
#include "systems/ServerDisconnectionSystem.h"
#include "systems/SimplifiedOnServeSystem.h"
#include "systems/StartCourseSystem.h"
#include "systems/TagShaderRender.h"
#include "systems/TestSystem.h"
#include "systems/ToastAnimationSystem.h"
#include "systems/ToastLifetimeSystem.h"
#include "systems/TooltipSystem.h"
#include "systems/TriggerDispatchSystem.h"
#include "systems/UnifiedAnimationSystem.h"
#include "systems/UpdateRenderTexture.h"
#include "systems/UpdateSpriteTransform.h"
#include "systems/battle_system_registry.h"
#include "testing/test_macros.h"
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
// Timing speed scale (default 1.0 = normal speed, higher = faster)
float render_backend::timing_speed_scale = 1.0f;
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
  auto &sophie = EntityHelper::createPermanentEntity(); // CRITICAL: Make sophie permanent so BattleProcessor singleton is never cleaned up

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
    make_network_manager(sophie);
    make_user_id_singleton();
    make_round_singleton(sophie);
    make_continue_game_request_singleton(sophie);
    make_continue_button_disabled_singleton(sophie);

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

    systems.register_update_system(std::make_unique<NetworkSystem>());
    systems.register_update_system(
        std::make_unique<ServerDisconnectionSystem>());
    systems.register_update_system(std::make_unique<ToastLifetimeSystem>());
    systems.register_update_system(std::make_unique<ToastAnimationSystem>());
    systems.register_update_system(std::make_unique<UpdateSpriteTransform>());
    systems.register_update_system(std::make_unique<UpdateShaderValues>());
    systems.register_update_system(std::make_unique<MarkEntitiesWithShaders>());
    systems.register_update_system(std::make_unique<HandleFreezeIconClick>());
    systems.register_update_system(std::make_unique<MarkIsHeldWhenHeld>());
    systems.register_update_system(std::make_unique<DropWhenNoLongerHeld>());
    battle_systems::register_battle_systems(systems);

    register_sound_systems(systems);
    register_ui_systems(systems);

    if (run_test.has_value()) {
      systems.register_update_system(std::make_unique<TestSystem>(*run_test));
    }
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
      systems.register_render_system(std::make_unique<RenderToastSystem>());
      systems.register_render_system(std::make_unique<RenderTooltipSystem>());
      systems.register_render_system(std::make_unique<RenderFPS>());
      systems.register_render_system(std::make_unique<RenderDebugWindowInfo>());
      systems.register_render_system(std::make_unique<EndDrawing>());
      //
    }

    if (!render_backend::is_headless_mode) {
      while (running && !raylib::WindowShouldClose()) {
        float dt = raylib::GetFrameTime();
        systems.run(dt * render_backend::timing_speed_scale);
      }
    } else {
      // Headless loop: run with fixed timestep; tests will exit the process
      while (running) {
        float dt = 1.0f / 60.0f;
        systems.run(dt * render_backend::timing_speed_scale);
      }
    }
  }
}

int main(int argc, char *argv[]) {

  // if nothing else ends up using this, we should move into preload.cpp
  argh::parser cmdl(argc, argv, argh::parser::PREFER_PARAM_FOR_UNREG_OPTION);

  if (cmdl["--help"]) {
    std::cout << "My Name Chef - Battle cooking game\n\n";
    std::cout << "Usage: my_name_chef [OPTIONS]\n\n";
    std::cout << "Options:\n";
    std::cout << "  --help                        Show this help message\n";
    std::cout << "  -w, --width <pixels>          Screen width (default: 1280)\n";
    std::cout << "  -h, --height <pixels>         Screen height (default: 720)\n";
    std::cout << "  --list-tests                  List all available tests\n";
    std::cout << "  --run-test <name>             Run a specific test\n";
    std::cout << "  --headless                    Enable headless mode (no rendering)\n";
    std::cout << "  --audit-strict                Enable strict side effect auditing\n";
    std::cout << "  --step-delay <ms>             Delay between test steps in milliseconds (default: 500)\n";
    std::cout << "  --timing-speed-scale <scale>  Battle timing speed multiplier (default: 1.0)\n";
    std::cout << "\n";
    std::cout << "Examples:\n";
    std::cout << "  my_name_chef --run-test validate_survivor_carryover --headless\n";
    std::cout << "  my_name_chef --width 1920 --height 1080\n";
    std::cout << "  my_name_chef --timing-speed-scale 2.0\n";
    return 0;
  }

  int screenWidth, screenHeight;
  cmdl({"-w", "--width"}, 1280) >> screenWidth;
  cmdl({"-h", "--height"}, 720) >> screenHeight;

  // Handle --list-tests flag (must come before other initialization)
  if (cmdl["--list-tests"]) {
    // Access test registry directly - static initializers from TEST() macros
    // should have already run when TestSystem.cpp was linked
    // No initialization needed - just list the tests and exit
    // Output format: one test name per line (for easy parsing)
    auto &registry = TestRegistry::get();
    auto test_list = registry.list_tests();

    for (const auto &name : test_list) {
      std::cout << name << std::endl;
    }

    return 0;
  }

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

  // Parse timing speed scale flag (overrides settings if provided)
  float timing_scale = Settings::get().get_battle_speed();
  if (cmdl({"--timing-speed-scale"}, timing_scale) >> timing_scale) {
    render_backend::timing_speed_scale = timing_scale;
    Settings::get().set_battle_speed(timing_scale);
    if (timing_scale != 1.0f) {
      log_info("TIMING SPEED SCALE: {}x (from command line)", timing_scale);
    }
  } else {
    render_backend::timing_speed_scale = timing_scale;
    if (timing_scale != 1.0f) {
      log_info("TIMING SPEED SCALE: {}x (from settings)", timing_scale);
    }
  }

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
