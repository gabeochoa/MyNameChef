#pragma once

#include "../components/combat_stats.h"
#include "../components/dish_battle_state.h"
#include "../components/dish_generation_ability.h"
#include "../components/dish_level.h"
#include "../components/has_render_order.h"
#include "../components/has_tooltip.h"
#include "../components/is_dish.h"
#include "../components/is_opponent_team_item.h"
#include "../components/is_player_team_item.h"
#include "../components/pending_dish_generation.h"
#include "../components/pre_battle_modifiers.h"
#include "../components/transform.h"
#include "../dish_types.h"
#include "../game_state_manager.h"
#include "../render_constants.h"
#include "../tooltip.h"
#include <afterhours/ah.h>

struct ProcessDishGenerationSystem : afterhours::System<PendingDishGeneration> {
  virtual bool should_run(float) override {
    GameStateManager &gsm = GameStateManager::get();
    return gsm.active_screen == GameStateManager::Screen::Battle;
  }

  void for_each_with(afterhours::Entity &pendingEntity,
                     PendingDishGeneration &pending, float dt) override {
    // Countdown delay
    pending.delayRemaining -= dt;
    if (pending.delayRemaining > 0.0f) {
      return;
    }

    // Find the generating entity
    afterhours::OptEntity opt_generatingEntity =
        afterhours::EntityHelper::getEntityForID(pending.generatingEntityId);

    if (!opt_generatingEntity) {
      log_warn("DISH_GEN: Generating entity {} not found, cleaning up pending "
               "generation",
               pending.generatingEntityId);
      pendingEntity.cleanup = true;
      return;
    }

    afterhours::Entity &generatingEntity = opt_generatingEntity.asE();

    // Get generation ability
    if (!generatingEntity.has<DishGenerationAbility>()) {
      log_warn(
          "DISH_GEN: Generating entity {} no longer has generation ability",
          pending.generatingEntityId);
      pendingEntity.cleanup = true;
      return;
    }

    DishGenerationAbility &ability =
        generatingEntity.get<DishGenerationAbility>();

    // Spawn the new dish
    spawn_generated_dish(ability, pending);

    // Clean up pending entity
    pendingEntity.cleanup = true;
  }

private:
  void spawn_generated_dish(const DishGenerationAbility &ability,
                            const PendingDishGeneration &pending) {
    // Create new dish entity
    afterhours::Entity &newDish = afterhours::EntityHelper::createEntity();

    // Calculate position
    float x = 120.0f + pending.targetSlot * 100.0f;
    float y = pending.isPlayerTeam ? 150.0f : 500.0f;

    log_info("DISH_GEN: Spawning {} at slot {} for {} team at ({}, {})",
             magic_enum::enum_name(ability.generatedDishType),
             pending.targetSlot, pending.isPlayerTeam ? "player" : "opponent",
             x, y);

    // Add basic components
    newDish.addComponent<Transform>(afterhours::vec2{x, y},
                                    afterhours::vec2{80.0f, 80.0f});
    newDish.addComponent<IsDish>(ability.generatedDishType);

    // Determine level
    int dishLevel = 1;
    if (ability.scalesWithLevel) {
      // Get level from generating entity
      afterhours::OptEntity opt_generatingEntity =
          afterhours::EntityHelper::getEntityForID(pending.generatingEntityId);
      if (opt_generatingEntity && opt_generatingEntity.asE().has<DishLevel>()) {
        dishLevel = opt_generatingEntity.asE().get<DishLevel>().level;
      }
      dishLevel += ability.levelBonus;
      dishLevel = std::max(1, dishLevel); // Minimum level 1
    }
    newDish.addComponent<DishLevel>(dishLevel);

    // Add battle state
    DishBattleState &dbs = newDish.addComponent<DishBattleState>();
    dbs.queue_index = pending.targetSlot;
    dbs.team_side = pending.isPlayerTeam ? DishBattleState::TeamSide::Player
                                         : DishBattleState::TeamSide::Opponent;
    dbs.phase = DishBattleState::Phase::InQueue; // Start in queue
    dbs.enter_progress = 0.0f;
    dbs.bite_timer = 0.0f;

    // Add combat components
    newDish.addComponent<CombatStats>();
    newDish.addComponent<PreBattleModifiers>();

    // Add rendering components
    newDish.addComponent<HasRenderOrder>(
        RenderOrder::BattleTeams, RenderScreen::Battle | RenderScreen::Results);

    // Add sprite
    auto dish_info = get_dish_info(ability.generatedDishType);
    const auto frame = afterhours::texture_manager::idx_to_sprite_frame(
        dish_info.sprite.i, dish_info.sprite.j);
    newDish.addComponent<afterhours::texture_manager::HasSprite>(
        afterhours::vec2{x, y}, afterhours::vec2{80.0f, 80.0f}, 0.f, frame,
        render_constants::kDishSpriteScale, raylib::Color{255, 255, 255, 255});

    // Add team markers
    if (pending.isPlayerTeam) {
      newDish.addComponent<IsPlayerTeamItem>();
    } else {
      newDish.addComponent<IsOpponentTeamItem>();
    }

    // Add tooltip
    newDish.addComponent<HasTooltip>(
        generate_dish_tooltip(ability.generatedDishType));

    log_info("DISH_GEN: Successfully spawned entity {} - {} (level {})",
             newDish.id, magic_enum::enum_name(ability.generatedDishType),
             dishLevel);
  }
};
