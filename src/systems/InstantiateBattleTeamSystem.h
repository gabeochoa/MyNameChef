#pragma once

#include "../components/battle_team_data.h"
#include "../components/battle_team_tags.h"
#include "../components/combat_queue.h"
#include "../components/dish_battle_state.h"
#include "../components/dish_level.h"
#include "../components/has_tooltip.h"
#include "../components/is_dish.h"
#include "../components/render_order.h"
#include "../components/transform.h"
#include "../dish_types.h"
#include "../game_state_manager.h"
#include "../render_backend.h"
#include "../render_constants.h"
#include "../tooltip.h"
#include <afterhours/ah.h>
#include <afterhours/src/plugins/texture_manager.h>
#include <magic_enum/magic_enum.hpp>

struct InstantiateBattleTeamSystem : afterhours::System<CombatQueue> {
  bool instantiated = false;
  GameStateManager::Screen last_screen = GameStateManager::Screen::Main;

  virtual bool should_run(float) override {
    auto &gsm = GameStateManager::get();

    if (last_screen == GameStateManager::Screen::Battle &&
        gsm.active_screen != GameStateManager::Screen::Battle) {
      instantiated = false;
    }

    last_screen = gsm.active_screen;
    return gsm.active_screen == GameStateManager::Screen::Battle &&
           !instantiated;
  }

  void for_each_with(afterhours::Entity &manager_entity, CombatQueue &,
                     float) override {
    bool player_ready = false;
    bool opponent_ready = false;

    if (manager_entity.has<BattleTeamDataPlayer>()) {
      const auto &data = manager_entity.get<BattleTeamDataPlayer>();
      if (!data.team.empty() && !data.instantiated) {
        instantiate_team(data.team, true);
        manager_entity.get<BattleTeamDataPlayer>().instantiated = true;
        player_ready = true;
      } else if (data.instantiated) {
        player_ready = true;
      }
    }

    if (manager_entity.has<BattleTeamDataOpponent>()) {
      const auto &data = manager_entity.get<BattleTeamDataOpponent>();
      if (!data.team.empty() && !data.instantiated) {
        instantiate_team(data.team, false);
        manager_entity.get<BattleTeamDataOpponent>().instantiated = true;
        opponent_ready = true;
      } else if (data.instantiated) {
        opponent_ready = true;
      }
    }

    if (player_ready && opponent_ready) {
      instantiated = true;
      afterhours::EntityHelper::merge_entity_arrays();
    }
  }

private:
  void instantiate_team(const std::vector<TeamDishSpec> &team_specs,
                        bool isPlayer) {
    for (const auto &spec : team_specs) {
      create_battle_dish_entity(spec, isPlayer);
    }
  }

  void create_battle_dish_entity(const TeamDishSpec &spec, bool isPlayer) {
    auto &entity = afterhours::EntityHelper::createEntity();

    float x, y;
    if (isPlayer) {
      x = 120.0f + spec.slot * 100.0f;
      y = 150.0f;
    } else {
      x = 120.0f + spec.slot * 100.0f;
      y = 500.0f;
    }

    log_info("BATTLE CREATE: Creating entity {} - Dish: {}, Player: {}, Slot: "
             "{}, Pos: ({}, {})",
             entity.id, magic_enum::enum_name(spec.dishType), isPlayer,
             spec.slot, x, y);

    entity.addComponent<Transform>(afterhours::vec2{x, y},
                                   afterhours::vec2{80.0f, 80.0f});
    entity.addComponent<IsDish>(spec.dishType);

    entity.addComponent<DishLevel>(spec.level);

    auto &dbs = entity.addComponent<DishBattleState>();
    dbs.queue_index = spec.slot;
    dbs.team_side = isPlayer ? DishBattleState::TeamSide::Player
                             : DishBattleState::TeamSide::Opponent;
    dbs.phase = DishBattleState::Phase::InQueue;
    dbs.enter_progress = 0.0f;
    dbs.bite_timer = 0.0f;

    entity.addComponent<HasRenderOrder>(
        RenderOrder::BattleTeams, RenderScreen::Battle | RenderScreen::Results);

    if (!render_backend::is_headless_mode) {
      auto dish_info = get_dish_info(spec.dishType);
      const auto frame = afterhours::texture_manager::idx_to_sprite_frame(
          dish_info.sprite.i, dish_info.sprite.j);
      entity.addComponent<afterhours::texture_manager::HasSprite>(
          afterhours::vec2{x, y}, afterhours::vec2{80.0f, 80.0f}, 0.f, frame,
          render_constants::kDishSpriteScale,
          raylib::Color{255, 255, 255, 255});
    }

    if (isPlayer) {
      entity.addComponent<IsPlayerTeamItem>();
    } else {
      entity.addComponent<IsOpponentTeamItem>();
    }

    entity.addComponent<HasTooltip>(generate_dish_tooltip(spec.dishType));
  }
};
