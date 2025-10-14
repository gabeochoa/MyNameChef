#include <afterhours/ah.h>
#include <fmt/format.h>

#include <afterhours/src/developer.h>
#include <afterhours/src/logging.h>

#include "../components/is_gallery_item.h"
#include "../components/is_shop_item.h"
#include "../game.h"
#include "../game_state_manager.h"
#include "../input_mapping.h"
#include "../render_constants.h"
#include "../settings.h"
#include "../shop.h"
#include "../systems/ExportMenuSnapshotSystem.h"
#include "../tooltip.h"
#include "../translation_manager.h"
#include "containers.h"
#include "controls.h"
#include "metrics.h"
#include "navigation.h"
#include <afterhours/src/plugins/texture_manager.h>

using namespace afterhours;

struct MapConfig;

using namespace afterhours::ui;
using namespace afterhours::ui::imm;
using namespace afterhours::ui::metrics;
using namespace afterhours::ui::controls;
using namespace afterhours::ui::containers;
using Screen = GameStateManager::Screen;

auto height_at_720p(float value) {
  return afterhours::ui::metrics::h720(value);
}
auto width_at_720p(float value) {
  return afterhours::ui::metrics::w1280(value);
}

static constexpr float kColumnWidth = 0.2f;
static constexpr float kContainerPadding = 0.02f;
static constexpr float kPadding720 = 5.f / 720.f;

struct SetupGameStylingDefaults
    : System<afterhours::ui::UIContext<InputAction>> {

  virtual void once(float) override {
    auto &styling_defaults = afterhours::ui::imm::UIStylingDefaults::get();

    styling_defaults
        .set_theme_color(afterhours::ui::Theme::Usage::Primary,
                         afterhours::Color{139, 69, 19, 255}) // Saddle brown
        .set_theme_color(afterhours::ui::Theme::Usage::Secondary,
                         afterhours::Color{34, 139, 34, 255}) // Forest green
        .set_theme_color(afterhours::ui::Theme::Usage::Accent,
                         afterhours::Color{255, 140, 0, 255}) // Dark orange
        .set_theme_color(afterhours::ui::Theme::Usage::Background,
                         afterhours::Color{64, 64, 64, 255}) // Dark gray
        .set_theme_color(afterhours::ui::Theme::Usage::Font,
                         afterhours::Color{245, 245, 220, 255}) // Beige
        .set_theme_color(afterhours::ui::Theme::Usage::DarkFont,
                         afterhours::Color{255, 255, 255,
                                           255}); // White for dark backgrounds

    // Set the default font for all components based on current language
    styling_defaults.set_default_font(
        get_font_name(translation_manager::get_font_for_language()), 16.f);

    // Component-specific styling
    styling_defaults.set_component_config(
        ComponentType::Button,
        ComponentConfig{}
            .with_padding(Padding{.top = screen_pct(5.f / 720.f),
                                  .left = pixels(0.f),
                                  .bottom = screen_pct(5.f / 720.f),
                                  .right = pixels(0.f)})
            .with_size(ComponentSize{screen_pct(200.f / 1280.f),
                                     screen_pct(50.f / 720.f)})
            .with_color_usage(Theme::Usage::Primary));

    styling_defaults.set_component_config(
        ComponentType::Slider,
        ComponentConfig{}
            .with_size(ComponentSize{screen_pct(200.f / 1280.f),
                                     screen_pct(50.f / 720.f)})
            .with_padding(Padding{.top = screen_pct(kPadding720),
                                  .left = pixels(0.f),
                                  .bottom = screen_pct(kPadding720),
                                  .right = pixels(0.f)})
            .with_color_usage(Theme::Usage::Secondary));

    styling_defaults.set_component_config(
        ComponentType::Checkbox,
        ComponentConfig{}
            .with_size(ComponentSize{screen_pct(200.f / 1280.f),
                                     screen_pct(50.f / 720.f)})
            .with_padding(Padding{.top = screen_pct(kPadding720),
                                  .left = pixels(0.f),
                                  .bottom = screen_pct(kPadding720),
                                  .right = pixels(0.f)})
            .with_color_usage(Theme::Usage::Primary));
  }
};

struct ScheduleMainMenuUI : System<afterhours::ui::UIContext<InputAction>> {

  virtual bool should_run(float) override {
    auto &gsm = GameStateManager::get();
    return gsm.is_menu_active() || gsm.is_game_active();
  }

  virtual void for_each_with(Entity &entity, UIContext<InputAction> &context,
                             float) override {
    auto &gsm = GameStateManager::get();
    gsm.update_screen();

    if (gsm.active_screen == Screen::Main) {
      gsm.active_screen = main_screen(entity, context);
      return;
    }
    if (gsm.active_screen == Screen::Settings) {
      gsm.active_screen = settings_screen(entity, context);
      return;
    }
    if (gsm.active_screen == Screen::Dishes) {
      gsm.active_screen = dishes_screen(entity, context);
      return;
    }
    if (gsm.active_screen == Screen::Shop) {
      gsm.active_screen = shop_screen(entity, context);
      return;
    }
    if (gsm.active_screen == Screen::Battle) {
      gsm.active_screen = battle_screen(entity, context);
      return;
    }
    if (gsm.active_screen == Screen::Results) {
      gsm.active_screen = results_screen(entity, context);
      return;
    }
    gsm.active_screen = gsm.active_screen;
  }

  Screen main_screen(Entity &entity, UIContext<InputAction> &context);
  Screen settings_screen(Entity &entity, UIContext<InputAction> &context);
  Screen dishes_screen(Entity &entity, UIContext<InputAction> &context);
  Screen shop_screen(Entity &entity, UIContext<InputAction> &context);
  Screen battle_screen(Entity &entity, UIContext<InputAction> &context);
  Screen results_screen(Entity &entity, UIContext<InputAction> &context);

  void exit_game() { running = false; }
};

// Reusable UI component functions
namespace ui_helpers {

// Reusable styled button component
ElementResult create_styled_button(UIContext<InputAction> &context,
                                   Entity &parent, const std::string &label,
                                   std::function<void()> on_click,
                                   int index = 0) {

  if (imm::button(context, mk(parent, index),
                  ComponentConfig{}.with_debug_name(label).with_label(label))) {
    on_click();
    return {true, parent};
  }

  return {false, parent};
}

// Reusable volume slider component
ElementResult create_volume_slider(UIContext<InputAction> &context,
                                   Entity &parent, const std::string &label,
                                   float &volume,
                                   std::function<void(float)> on_change,
                                   int index = 0) {

  if (auto result = slider(context, mk(parent, index), volume,
                           ComponentConfig{}.with_label(label),
                           SliderHandleValueLabelPosition::OnHandle)) {
    volume = result.as<float>();
    on_change(volume);
    return {true, parent};
  }

  return {false, parent};
}

// Reusable screen container component
ElementResult create_screen_container(UIContext<InputAction> &context,
                                      Entity &parent,
                                      const std::string &debug_name) {

  return imm::div(
      context, mk(parent),
      ComponentConfig{}
          .with_size(ComponentSize{screen_pct(1.f), screen_pct(1.f)})
          .with_absolute_position()
          .with_debug_name(debug_name));
}

// Reusable control group component
ElementResult create_column_container(UIContext<InputAction> &context,
                                      Entity &parent,
                                      const std::string &debug_name,
                                      int index = 0) {

  return imm::div(
      context, mk(parent, index),
      ComponentConfig{}
          .with_size(ComponentSize{screen_pct(kColumnWidth), screen_pct(1.f)})
          .with_padding(Padding{.top = screen_pct(kContainerPadding),
                                .left = screen_pct(kContainerPadding)})
          .with_flex_direction(FlexDirection::Column)
          .with_debug_name(debug_name));
}

} // namespace ui_helpers

Screen ScheduleMainMenuUI::main_screen(Entity &entity,
                                       UIContext<InputAction> &context) {
  auto elem = ui_helpers::create_screen_container(context, entity, "screen");

  // Add a background
  auto bg =
      imm::div(context, mk(elem.ent()),
               ComponentConfig{}
                   .with_size(ComponentSize{screen_pct(1.f), screen_pct(1.f)})
                   .with_color_usage(Theme::Usage::Background)
                   .with_debug_name("main_background")
                   .with_rounded_corners(RoundedCorners().all_sharp()));

  auto top_left =
      column_left<InputAction>(context, bg.ent(), "main_top_left", 0);

  // Play button
  button_labeled<InputAction>(
      context, top_left.ent(), "Play",
      []() { GameStateManager::get().start_game(); }, 0);

  // Settings button
  button_labeled<InputAction>(
      context, top_left.ent(), "Settings",
      []() { navigation::to(GameStateManager::Screen::Settings); }, 1);

  // Dishes button
  button_labeled<InputAction>(
      context, top_left.ent(), "Dishes",
      []() { navigation::to(GameStateManager::Screen::Dishes); }, 2);

  // Exit button
  button_labeled<InputAction>(
      context, top_left.ent(), "Quit", [this]() { exit_game(); }, 3);

  return GameStateManager::get().next_screen.value_or(
      GameStateManager::get().active_screen);
}

Screen ScheduleMainMenuUI::dishes_screen(Entity &entity,
                                         UIContext<InputAction> &context) {
  auto elem = ui_helpers::create_screen_container(context, entity, "screen");

  // Do NOT draw a fullscreen background here; it would cover world sprites.
  auto top_left =
      column_left<InputAction>(context, elem.ent(), "dishes_top_left", 0);

  button_labeled<InputAction>(
      context, top_left.ent(), "Back", []() { navigation::back(); }, 0);

  return GameStateManager::get().next_screen.value_or(
      GameStateManager::get().active_screen);
}

Screen ScheduleMainMenuUI::settings_screen(Entity &entity,
                                           UIContext<InputAction> &context) {
  auto elem = ui_helpers::create_screen_container(context, entity, "screen");
  auto top_left =
      column_left<InputAction>(context, elem.ent(), "settings_top_left", 0);
  {
    button_labeled<InputAction>(
        context, top_left.ent(), "Back",
        []() {
          Settings::get().update_resolution(
              EntityHelper::get_singleton_cmp<
                  window_manager::ProvidesCurrentResolution>()
                  ->current_resolution);
          navigation::back();
        },
        0);
  }

  // Master volume slider
  {
    float master_volume = Settings::get().get_master_volume();
    slider_labeled<InputAction>(
        context, top_left.ent(), "Master Volume", master_volume,
        [](float volume) { Settings::get().update_master_volume(volume); }, 1);
  }

  // Music volume slider
  {
    float music_volume = Settings::get().get_music_volume();
    slider_labeled<InputAction>(
        context, top_left.ent(), "Music Volume", music_volume,
        [](float volume) { Settings::get().update_music_volume(volume); }, 2);
  }

  // SFX volume slider
  {
    float sfx_volume = Settings::get().get_sfx_volume();
    slider_labeled<InputAction>(
        context, top_left.ent(), "SFX Volume", sfx_volume,
        [](float volume) { Settings::get().update_sfx_volume(volume); }, 3);
  }

  // Fullscreen checkbox
  if (checkbox_labeled<InputAction>(context, top_left.ent(), "Fullscreen",
                                    Settings::get().get_fullscreen_enabled(),
                                    4)) {
    Settings::get().toggle_fullscreen();
  }

  // Post Processing checkbox
  if (checkbox_labeled<InputAction>(
          context, top_left.ent(), "Post Processing",
          Settings::get().get_post_processing_enabled(), 5)) {
    Settings::get().toggle_post_processing();
  }

  return GameStateManager::get().next_screen.value_or(
      GameStateManager::get().active_screen);
}

Screen ScheduleMainMenuUI::shop_screen(Entity &entity,
                                       UIContext<InputAction> &context) {
  auto elem = ui_helpers::create_screen_container(context, entity, "screen");

  auto top_right =
      div(context, mk(elem.ent()),
          ComponentConfig{}
              .with_size(ComponentSize{screen_pct(0.8f), screen_pct(0.2f)})
              .with_margin(Spacing::sm)
              .with_flex_direction(FlexDirection::Row)
              .with_debug_name("shop_buttons_row"));

  // Create Next Round button
  button_labeled<InputAction>(
      context, top_right.ent(), "Next Round",
      []() {
        // Export menu snapshot
        ExportMenuSnapshotSystem export_system;
        std::string filename = export_system.export_menu_snapshot();

        if (!filename.empty()) {
          // Navigate to battle screen
          GameStateManager::get().to_battle();
        }
      },
      1);

  div(context, mk(top_right.ent()),
      ComponentConfig{}
          .with_size(ComponentSize{screen_pct(0.4f), screen_pct(0.1f)})
          .with_margin(
              Margin{.left = screen_pct(0.6f), .top = screen_pct(0.05f)})
          .with_flex_direction(FlexDirection::Row)
          .with_debug_name("shop_buttons_row"));

  // Reroll button (cost 5 gold): delete all shop items and regenerate
  button_labeled<InputAction>(
      context, top_right.ent(), "Reroll (5)",
      []() {
        auto wallet_entity = EntityHelper::get_singleton<Wallet>();
        if (!wallet_entity.get().has<Wallet>())
          return;
        auto &wallet = wallet_entity.get().get<Wallet>();
        if (wallet.gold < 5)
          return;
        wallet.gold -= 5;

        // Mark existing shop items for cleanup
        for (auto &ref : afterhours::EntityQuery({.force_merge = true})
                             .template whereHasComponent<IsShopItem>()
                             .gen()) {
          ref.get().cleanup = true;
        }

        // Force cleanup of marked entities before creating new ones
        afterhours::EntityHelper::cleanup();

        // Regenerate shop items for all slots
        // Get current shop tier
        auto shop_tier_entity = EntityHelper::get_singleton<ShopTier>();
        int current_tier = 1; // Default to tier 1
        if (shop_tier_entity.get().has<ShopTier>()) {
          current_tier = shop_tier_entity.get().get<ShopTier>().current_tier;
        }

        for (int slot = 0; slot < SHOP_SLOTS; ++slot) {
          make_shop_item(slot, get_random_dish_for_tier(current_tier));
        }
        afterhours::EntityHelper::merge_entity_arrays();
      },
      0);

  return GameStateManager::get().next_screen.value_or(
      GameStateManager::get().active_screen);
}

Screen ScheduleMainMenuUI::battle_screen(Entity &entity,
                                         UIContext<InputAction> &context) {
  auto elem = ui_helpers::create_screen_container(context, entity, "screen");
  auto top_left =
      column_left<InputAction>(context, elem.ent(), "battle_top_left", 0);

  // Create Skip to Results button
  button_labeled<InputAction>(
      context, top_left.ent(), "Skip to Results",
      []() { GameStateManager::get().to_results(); }, 0);

  return GameStateManager::get().next_screen.value_or(
      GameStateManager::get().active_screen);
}

Screen ScheduleMainMenuUI::results_screen(Entity &entity,
                                          UIContext<InputAction> &context) {
  auto elem = ui_helpers::create_screen_container(context, entity, "screen");

  // Add a background
  auto bg =
      imm::div(context, mk(elem.ent()),
               ComponentConfig{}
                   .with_size(ComponentSize{screen_pct(1.f), screen_pct(1.f)})
                   .with_color_usage(Theme::Usage::Background)
                   .with_debug_name("results_background")
                   .with_rounded_corners(RoundedCorners().all_sharp()));

  auto top_left =
      column_left<InputAction>(context, bg.ent(), "results_top_left", 0);

  // Create Back to Shop button
  button_labeled<InputAction>(
      context, top_left.ent(), "Back to Shop",
      []() {
        log_info("Back to Shop button clicked!");
        GameStateManager::get().set_next_screen(Screen::Shop);
      },
      0);

  return GameStateManager::get().next_screen.value_or(
      GameStateManager::get().active_screen);
}

void register_ui_systems(afterhours::SystemManager &systems) {
  ui::register_before_ui_updates<InputAction>(systems);
  {
    systems.register_update_system(
        std::make_unique<SetupGameStylingDefaults>());
    systems.register_update_system(std::make_unique<NavigationSystem>());
    systems.register_update_system(std::make_unique<ScheduleMainMenuUI>());
  }
  ui::register_after_ui_updates<InputAction>(systems);
}

void enforce_ui_singletons(afterhours::SystemManager &systems) {
  systems.register_update_system(
      std::make_unique<
          afterhours::developer::EnforceSingleton<MenuNavigationStack>>());
}

void add_ui_singleton_components(afterhours::Entity &entity) {
  entity.addComponent<MenuNavigationStack>();
  afterhours::EntityHelper::registerSingleton<MenuNavigationStack>(entity);
}
