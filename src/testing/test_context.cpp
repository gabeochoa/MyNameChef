#include "test_context.h"
#include "../game.h"
#include "../shop.h"
#include "../ui/ui_systems.h"
#include <raylib/raylib.h>

// Provide definitions for globals expected by game code when not linking
// main.cpp
bool running = true;
raylib::RenderTexture2D mainRT = {};
raylib::RenderTexture2D screenRT = {};

TestContext::TestContext() {}
TestContext::~TestContext() { shutdown(); }

void TestContext::initialize(Mode mode) {
  if (initialized)
    return;
  currentMode = mode;

  if (currentMode == Mode::Interactive) {
    raylib::InitWindow(1280, 720, "My Name Chef - Tests");
    raylib::SetTargetFPS(60);
  } else {
    // Headless: still init a window but hidden to let raylib run
    raylib::SetConfigFlags(raylib::FLAG_WINDOW_HIDDEN);
    raylib::InitWindow(1280, 720, "My Name Chef - Headless");
    raylib::SetTargetFPS(60);
  }

  Settings::get().load_save_file(1280, 720);
  Preload::get().init("My Name Chef Tests").make_singleton();
  Settings::get().refresh_settings();

  setup_singletons();
  rootEntity = &afterhours::EntityHelper::createEntity();

  // Provide globals expected by render/ui/navigation
  running = true;
  mainRT = raylib::LoadRenderTexture(Settings::get().get_screen_width(),
                                     Settings::get().get_screen_height());
  screenRT = raylib::LoadRenderTexture(Settings::get().get_screen_width(),
                                       Settings::get().get_screen_height());

  // Register core update systems similar to main
  afterhours::input::register_update_systems(systems);
  afterhours::window_manager::register_update_systems(systems);
  register_ui_systems(systems);
  register_shop_update_systems(systems);
  initialized = true;
}

void TestContext::shutdown() {
  if (!initialized)
    return;
  initialized = false;
  teardown_window();
}

void TestContext::setup_singletons() {
  afterhours::window_manager::enforce_singletons(systems);
  afterhours::ui::enforce_singletons<InputAction>(systems);
  afterhours::input::enforce_singletons(systems);
  afterhours::texture_manager::enforce_singletons(systems);
  enforce_ui_singletons(systems);
  // game state singletons used by UI/screens
  if (rootEntity == nullptr) {
    rootEntity = &afterhours::EntityHelper::createEntity();
  }
  make_shop_manager(*rootEntity);
  make_combat_manager(*rootEntity);
  make_battle_processor_manager(*rootEntity);
}

void TestContext::teardown_window() {
  if (!raylib::WindowShouldClose()) {
    // drain a couple frames
    for (int i = 0; i < 2; ++i) {
      render_backend::BeginDrawing();
      render_backend::ClearBackground(raylib::BLACK);
      render_backend::EndDrawing();
    }
  }
  raylib::CloseWindow();
}

void TestContext::pump_once(float dt) {
  systems.run(dt);
  if (currentMode == Mode::Interactive) {
    render_backend::BeginDrawing();
    render_backend::ClearBackground(raylib::RAYWHITE);
    systems.render(afterhours::EntityHelper::get_entities(), dt);
    render_backend::EndDrawing();
  }
}

void TestContext::run_for_seconds(float seconds) {
  const float dt = 1.0f / 60.0f;
  float t = 0.0f;
  while (t < seconds && !raylib::WindowShouldClose()) {
    pump_once(dt);
    t += dt;
  }
}

std::vector<std::string> TestContext::list_buttons() {
  std::vector<std::string> labels;
  for (auto &ep : afterhours::EntityHelper::get_entities()) {
    auto &e = *ep;
    if (e.has<afterhours::ui::UIComponent>() &&
        e.has<afterhours::ui::HasClickListener>()) {
      std::string name;
      if (e.has<afterhours::ui::HasLabel>()) {
        name = e.get<afterhours::ui::HasLabel>().label;
      }
      if (name.empty() && e.has<afterhours::ui::UIComponentDebug>()) {
        name = e.get<afterhours::ui::UIComponentDebug>().name();
      }
      if (!name.empty())
        labels.push_back(name);
    }
  }
  return labels;
}

bool TestContext::click_button(const std::string &label) {
  for (auto &ep : afterhours::EntityHelper::get_entities()) {
    auto &e = *ep;
    if (e.has<afterhours::ui::UIComponent>() &&
        e.has<afterhours::ui::HasClickListener>()) {
      std::string name;
      if (e.has<afterhours::ui::HasLabel>()) {
        name = e.get<afterhours::ui::HasLabel>().label;
      }
      if (name.empty() && e.has<afterhours::ui::UIComponentDebug>()) {
        name = e.get<afterhours::ui::UIComponentDebug>().name();
      }
      if (name == label) {
        e.get<afterhours::ui::HasClickListener>().cb(e);
        return true;
      }
    }
  }
  return false;
}

bool TestContext::wait_for_screen(GameStateManager::Screen screen,
                                  float timeout_seconds) {
  auto start = std::chrono::steady_clock::now();
  const float dt = 1.0f / 60.0f;
  while (!raylib::WindowShouldClose()) {
    if (GameStateManager::get().active_screen == screen)
      return true;
    pump_once(dt);
    auto now = std::chrono::steady_clock::now();
    auto ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(now - start);
    if (ms.count() > static_cast<int>(timeout_seconds * 1000.0f))
      break;
  }
  return GameStateManager::get().active_screen == screen;
}
