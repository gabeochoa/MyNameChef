#pragma once

#include "../components/combat_stats.h"
#include "../components/dish_battle_state.h"
#include "../components/dish_generation_ability.h"
#include "../components/pending_dish_generation.h"
#include "../dish_types.h"
#include "../game_state_manager.h"
#include <afterhours/ah.h>

struct DishGenerationSystem
    : afterhours::System<DishGenerationAbility, DishBattleState> {
  virtual bool should_run(float) override {
    GameStateManager &gsm = GameStateManager::get();
    return gsm.active_screen == GameStateManager::Screen::Battle;
  }

  void for_each_with(afterhours::Entity &e, DishGenerationAbility &ability,
                     DishBattleState &dbs, float dt) override {
    // Check if ability can still trigger
    if (ability.currentGenerations >= ability.maxGenerations) {
      return;
    }

    // Check trigger conditions
    if (should_trigger_generation(ability, dbs, e)) {
      trigger_generation(e, ability, dbs);
    }
  }

private:
  bool should_trigger_generation(const DishGenerationAbility &ability,
                                 const DishBattleState &dbs,
                                 const afterhours::Entity &e) {
    switch (ability.trigger) {
    case GenerationTrigger::OnEnterCombat:
      return dbs.phase == DishBattleState::Phase::InCombat &&
             dbs.enter_progress >= 1.0f;

    case GenerationTrigger::OnFirstBite:
      // This would need to track if first bite occurred
      return false; // TODO: Implement bite tracking

    case GenerationTrigger::OnTakeDamage:
      // This would need to track damage taken
      return false; // TODO: Implement damage tracking

    case GenerationTrigger::OnDealDamage:
      // This would need to track damage dealt
      return false; // TODO: Implement damage tracking

    case GenerationTrigger::OnDefeat:
      return dbs.phase == DishBattleState::Phase::Finished;

    case GenerationTrigger::OnVictory:
      // This would need to track if opponent was defeated
      return false; // TODO: Implement victory tracking

    case GenerationTrigger::EveryTick:
      return dbs.phase == DishBattleState::Phase::InCombat;

    case GenerationTrigger::EveryNBites:
      // This would need to track bite count
      return false; // TODO: Implement bite counting

    default:
      return false;
    }
  }

  void trigger_generation(afterhours::Entity &generatingEntity,
                          DishGenerationAbility &ability,
                          DishBattleState &dbs) {
    // Increment generation count
    ability.currentGenerations++;

    // Create pending generation entity
    afterhours::Entity &pendingEntity =
        afterhours::EntityHelper::createEntity();
    PendingDishGeneration &pending =
        pendingEntity.addComponent<PendingDishGeneration>();

    pending.generatingEntityId = generatingEntity.id;
    pending.delayRemaining = ability.delaySeconds;
    pending.isPlayerTeam = (dbs.team_side == DishBattleState::TeamSide::Player);

    // Determine target slot based on position
    pending.targetSlot =
        determine_target_slot(ability.position, dbs, pending.isPlayerTeam);

    log_info("DISH_GEN: Triggered generation of {} by entity {} at slot {}",
             magic_enum::enum_name(ability.generatedDishType),
             generatingEntity.id, pending.targetSlot);
  }

  int determine_target_slot(GenerationPosition position,
                            const DishBattleState &dbs, bool isPlayerTeam) {
    switch (position) {
    case GenerationPosition::EndOfQueue:
      return find_last_occupied_slot(isPlayerTeam) + 1;

    case GenerationPosition::InSitu:
      return dbs.queue_index;

    case GenerationPosition::FrontOfQueue:
      return 0; // Always front

    case GenerationPosition::AdjacentSlots:
      return find_adjacent_empty_slot(dbs.queue_index, isPlayerTeam);

    default:
      return dbs.queue_index;
    }
  }

  int find_last_occupied_slot(bool isPlayerTeam) {
    int lastSlot = -1;
    for (afterhours::Entity &e :
         afterhours::EntityQuery().whereHasComponent<DishBattleState>().gen()) {
      DishBattleState &otherDbs = e.get<DishBattleState>();
      if ((otherDbs.team_side == DishBattleState::TeamSide::Player) ==
          isPlayerTeam) {
        lastSlot = std::max(lastSlot, otherDbs.queue_index);
      }
    }
    return lastSlot;
  }

  int find_adjacent_empty_slot(int centerSlot, bool isPlayerTeam) {
    // Check slots around the center slot
    for (int offset = 1; offset <= 3; offset++) {
      for (int direction = -1; direction <= 1; direction += 2) {
        int testSlot = centerSlot + (offset * direction);
        if (testSlot >= 0 && is_slot_empty(testSlot, isPlayerTeam)) {
          return testSlot;
        }
      }
    }
    return centerSlot; // Fallback to same slot
  }

  bool is_slot_empty(int slot, bool isPlayerTeam) {
    for (afterhours::Entity &e :
         afterhours::EntityQuery().whereHasComponent<DishBattleState>().gen()) {
      DishBattleState &otherDbs = e.get<DishBattleState>();
      if ((otherDbs.team_side == DishBattleState::TeamSide::Player) ==
              isPlayerTeam &&
          otherDbs.queue_index == slot) {
        return false;
      }
    }
    return true;
  }
};
