#pragma once

#include "../components/battle_anim_keys.h"
#include "../components/combat_stats.h"
#include "../components/deferred_flavor_mods.h"
#include "../components/dish_battle_state.h"
#include "../components/dish_level.h"
#include "../components/is_dish.h"
#include "../components/is_inventory_item.h"
#include "../components/is_shop_item.h"
#include "../components/pre_battle_modifiers.h"
#include "../components/render_order.h"
#include "../components/transform.h"
#include "../dish_types.h"
#include "../game_state_manager.h"
#include "../rl.h"
#include <afterhours/ah.h>
#include <afterhours/src/plugins/animation.h>

// Draw Zing/Body badges on top of dish sprites (top-left: Zing, top-right:
// Body)
struct RenderZingBodyOverlay : afterhours::System<HasRenderOrder, IsDish> {
  virtual bool should_run(float) override {
    auto &gsm = GameStateManager::get();
    return GameStateManager::should_render_world_entities(gsm.active_screen);
  }

  virtual void for_each_with(const afterhours::Entity &entity,
                             const HasRenderOrder &render_order,
                             const IsDish &is_dish, float) const override {
    // Do not render overlays for entities marked for cleanup
    if (entity.cleanup)
      return;

    bool is_battle_dish = entity.has<DishBattleState>();
    bool is_shop_item = entity.has<IsShopItem>();
    bool is_inventory_item = entity.has<IsInventoryItem>();

    if (!is_battle_dish && !is_shop_item && !is_inventory_item)
      return;

    // Respect screen render flags
    auto &gsm = GameStateManager::get();
    RenderScreen current_screen = static_cast<RenderScreen>(
        GameStateManager::render_screen_for(gsm.active_screen));
    if (!render_order.should_render_on_screen(current_screen))
      return;

    // On battle screen, skip overlays for finished dishes
    if (is_battle_dish && GameStateManager::get().active_screen ==
                              GameStateManager::Screen::Battle) {
      const DishBattleState &dbs_check = entity.get<DishBattleState>();
      if (dbs_check.phase == DishBattleState::Phase::Finished)
        return;
    }
    if (!entity.has<Transform>())
      return;
    const auto &transform = entity.get<Transform>();

    // Compute on-screen rect including battle animation offsets so the overlay
    // follows the moving sprite
    float offset_x = 0.0f;
    float offset_y = 0.0f;

    if (is_battle_dish) {
      const auto &dbs = entity.get<DishBattleState>();
      bool isPlayer = dbs.team_side == DishBattleState::TeamSide::Player;

      // Slide-in animation (vertical)
      float slide_v = 1.0f;
      if (auto v = afterhours::animation::get_value(BattleAnimKey::SlideIn,
                                                    (size_t)entity.id);
          v.has_value()) {
        slide_v = std::clamp(v.value(), 0.0f, 1.0f);
      }

      if (isPlayer) {
        float off = -(transform.position.y + transform.size.y + 20.0f);
        offset_y = (1.0f - slide_v) * off;
      } else {
        float screen_h = raylib::GetScreenHeight();
        float off = (screen_h - transform.position.y) + 20.0f;
        offset_y = (1.0f - slide_v) * off;
      }

      // While entering, mirror the RenderBattleTeams midline ease so overlays
      // stay glued to the sprite. Once InCombat, position remains at
      // snapped transform.
      if (dbs.phase == DishBattleState::Phase::Entering &&
          dbs.enter_progress >= 0.0f) {
        float present_v = std::clamp(dbs.enter_progress, 0.0f, 1.0f);
        float judge_center_y = 360.0f;
        offset_y += (judge_center_y - transform.position.y) * present_v;
      }

      // In head-to-head, avoid overlap by separating slightly on the Y axis
      if (dbs.phase == DishBattleState::Phase::InCombat) {
        const float headToHeadYSeparation = std::max(24.0f, transform.size.y * 0.12f);
        offset_y += isPlayer ? -headToHeadYSeparation : headToHeadYSeparation;
      }
    }

    int zing = 0;
    int body = 0;
    bool has_deferred = entity.has<DeferredFlavorMods>();
    bool has_pre_battle = entity.has<PreBattleModifiers>();
    bool is_in_combat = is_battle_dish && entity.get<DishBattleState>().phase ==
                                              DishBattleState::Phase::InCombat;

    // Use CombatStats if available (whether in combat or not) to ensure consistency
    // with ComputeCombatStatsSystem calculations
    if (is_battle_dish && entity.has<CombatStats>()) {
      const auto &cs = entity.get<CombatStats>();
      // Gate "current" view on animation completion to avoid premature drop
      bool slide_done = true;
      if (auto v = afterhours::animation::get_value(BattleAnimKey::SlideIn,
                                                    (size_t)entity.id);
          v.has_value()) {
        slide_done = v.value() >= 1.0f;
      }
      bool enter_done = true;
      if (is_battle_dish) {
        const auto &dbsGate = entity.get<DishBattleState>();
        enter_done = dbsGate.enter_progress >= 1.0f;
      }
      if (is_in_combat && slide_done && enter_done) {
        zing = cs.currentZing;
        body = cs.currentBody;
      } else {
        // Not yet fully presented or not in combat: show base
        zing = cs.baseZing;
        body = cs.baseBody;
      }
    } else {
      auto dish_info = get_dish_info(is_dish.type);
      FlavorStats flavor = dish_info.flavor;
      if (has_deferred) {
        const auto &def = entity.get<DeferredFlavorMods>();
        flavor.satiety += def.satiety;
        flavor.sweetness += def.sweetness;
        flavor.spice += def.spice;
        flavor.acidity += def.acidity;
        flavor.umami += def.umami;
        flavor.richness += def.richness;
        flavor.freshness += def.freshness;
      }

      zing = flavor.zing();
      body = flavor.body();

      if (entity.has<DishLevel>()) {
        const auto &lvl = entity.get<DishLevel>();
        if (lvl.level > 1) {
          int level_multiplier = 1 << (lvl.level - 1);
          zing *= level_multiplier;
          body *= level_multiplier;
        }
      }

      if (has_pre_battle) {
        const auto &pre = entity.get<PreBattleModifiers>();
        zing += pre.zingDelta;
        body += pre.bodyDelta;
      }

      zing = std::max(1, zing);
      body = std::max(0, body);
    }

    // Badge sizes relative to sprite rect
    const Rectangle rect = Rectangle{transform.position.x + offset_x,
                                     transform.position.y + offset_y,
                                     transform.size.x, transform.size.y};
    const float badgeSize =
        std::max(18.0f, std::min(rect.width, rect.height) * 0.26f);
    const float padding = 5.0f;

    // Zing: green rhombus (diamond) top-left
    const float zx = rect.x + padding + badgeSize * 0.5f;
    const float zy = rect.y + padding + badgeSize * 0.5f;
    // Zing: red
    raylib::Color zingColor = raylib::Color{200, 40, 40, 245};
    // Draw as a rotated square to guarantee a full diamond
    Rectangle diamond{zx, zy, badgeSize, badgeSize};
    render_backend::DrawRectanglePro(
        diamond, vec2{badgeSize * 0.5f, badgeSize * 0.5f}, 45.0f, zingColor);

    // Zing number (supports up to two digits)
    const int fontSize = static_cast<int>(badgeSize * 0.72f);
    const std::string zingText = std::to_string(zing);
    const int zw = raylib::MeasureText(zingText.c_str(), fontSize);
    render_backend::DrawText(zingText.c_str(), static_cast<int>(zx - zw / 2.0f),
                             static_cast<int>(zy - fontSize / 2.0f), fontSize,
                             raylib::BLACK);

    // Body: pale yellow square top-right
    const float bx = rect.x + rect.width - padding - badgeSize;
    const float by = rect.y + padding;
    // Body: green
    raylib::Color bodyColor = raylib::Color{30, 160, 70, 245};
    render_backend::DrawRectangle(static_cast<int>(bx), static_cast<int>(by),
                                  static_cast<int>(badgeSize),
                                  static_cast<int>(badgeSize), bodyColor);

    const std::string bodyText = std::to_string(body);
    const int bw = raylib::MeasureText(bodyText.c_str(), fontSize);
    const int bh = fontSize;
    render_backend::DrawText(bodyText.c_str(),
                             static_cast<int>(bx + (badgeSize - bw) * 0.5f),
                             static_cast<int>(by + (badgeSize - bh) * 0.5f),
                             fontSize, raylib::BLACK);
  }
};
