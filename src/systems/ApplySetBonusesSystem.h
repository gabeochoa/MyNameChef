#pragma once

#include "../components/applied_set_bonuses.h"
#include "../components/battle_synergy_counts.h"
#include "../components/cuisine_tag.h"
#include "../components/dish_battle_state.h"
#include "../components/dish_effect.h"
#include "../components/is_dish.h"
#include "../components/persistent_combat_modifiers.h"
#include "../components/set_bonus_definitions.h"
#include "../components/synergy_bonus_effects.h"
#include "../game_state_manager.h"
#include "../query.h"
#include "../shop.h"
#include <afterhours/ah.h>
#include <magic_enum/magic_enum.hpp>

struct ApplySetBonusesSystem : afterhours::System<> {
  bool applied = false;
  GameStateManager::Screen last_screen = GameStateManager::Screen::Main;

  virtual bool should_run(float) override {
    auto &gsm = GameStateManager::get();

    if (last_screen == GameStateManager::Screen::Battle &&
        gsm.active_screen != GameStateManager::Screen::Battle) {
      applied = false;
    }

    last_screen = gsm.active_screen;
    return gsm.active_screen == GameStateManager::Screen::Battle && !applied;
  }

  void once(float) override {
    afterhours::RefEntity battle_synergy_entity =
        afterhours::EntityHelper::get_singleton<BattleSynergyCounts>();
    afterhours::RefEntity applied_bonuses_entity =
        afterhours::EntityHelper::get_singleton<AppliedSetBonuses>();

    const BattleSynergyCounts &battle_synergy =
        battle_synergy_entity.get().get<BattleSynergyCounts>();
    AppliedSetBonuses &applied_bonuses =
        applied_bonuses_entity.get().get<AppliedSetBonuses>();

    const std::map<CuisineTagType, std::map<int, SetBonusDefinition>>
        &bonus_defs = synergy_bonuses::get_cuisine_bonus_definitions();

    apply_bonuses_for_team(battle_synergy.player_cuisine_counts,
                           DishBattleState::TeamSide::Player, bonus_defs,
                           applied_bonuses);
    apply_bonuses_for_team(battle_synergy.opponent_cuisine_counts,
                           DishBattleState::TeamSide::Opponent, bonus_defs,
                           applied_bonuses);

    applied = true;
  }

private:
  void apply_bonuses_for_team(
      const std::map<CuisineTagType, int> &counts,
      DishBattleState::TeamSide team_side,
      const std::map<CuisineTagType, std::map<int, SetBonusDefinition>>
          &bonus_defs,
      AppliedSetBonuses &applied_bonuses) {

    for (const auto &[cuisine, count] : counts) {
      std::map<CuisineTagType,
               std::map<int, SetBonusDefinition>>::const_iterator cuisine_it =
          bonus_defs.find(cuisine);
      if (cuisine_it == bonus_defs.end()) {
        continue;
      }

      const std::map<int, SetBonusDefinition> &thresholds = cuisine_it->second;
      const std::vector<int> threshold_values = {2, 4, 6};

      for (int threshold : threshold_values) {
        if (count < threshold) {
          continue;
        }

        std::map<int, SetBonusDefinition>::const_iterator threshold_it =
            thresholds.find(threshold);
        if (threshold_it == thresholds.end()) {
          continue;
        }

        std::pair<CuisineTagType, int> key = {cuisine, threshold};
        if (applied_bonuses.applied_cuisine.find(key) !=
            applied_bonuses.applied_cuisine.end()) {
          continue;
        }

        const auto &bonus = threshold_it->second;
        apply_bonus_to_targets(bonus, team_side);
        applied_bonuses.applied_cuisine.insert(key);
      }
    }
  }

  void apply_bonus_to_targets(const SetBonusDefinition &bonus,
                              DishBattleState::TeamSide source_team_side) {
    const DishBattleState::TeamSide opposite_side =
        (source_team_side == DishBattleState::TeamSide::Player)
            ? DishBattleState::TeamSide::Opponent
            : DishBattleState::TeamSide::Player;

    switch (bonus.targetScope) {
    case TargetScope::AllAllies: {
      apply_bonus_to_entities(EQ().whereHasComponent<IsDish>()
                                  .whereHasComponent<DishBattleState>()
                                  .whereTeamSide(source_team_side)
                                  .gen(),
                              bonus);
      break;
    }

    case TargetScope::AllOpponents: {
      apply_bonus_to_entities(EQ().whereHasComponent<IsDish>()
                                  .whereHasComponent<DishBattleState>()
                                  .whereTeamSide(opposite_side)
                                  .gen(),
                              bonus);
      break;
    }

    case TargetScope::Self:
    case TargetScope::Opponent:
    case TargetScope::DishesAfterSelf:
    case TargetScope::FutureAllies:
    case TargetScope::FutureOpponents:
    case TargetScope::Previous:
    case TargetScope::Next:
    case TargetScope::SelfAndAdjacent:
    case TargetScope::RandomAlly:
    case TargetScope::RandomOpponent:
    case TargetScope::RandomDish:
    case TargetScope::RandomOtherAlly:
    default:
      break;
    }
  }

  void apply_bonus_to_entities(const afterhours::RefEntities &entities,
                               const SetBonusDefinition &bonus) {
    for (afterhours::RefEntity entity_ref : entities) {
      apply_bonus_to_entity(entity_ref.get(), bonus);
    }
  }

  void apply_bonus_to_entity(afterhours::Entity &target,
                             const SetBonusDefinition &bonus) {
    if (bonus.zingDelta != 0 || bonus.bodyDelta != 0) {
      if (!target.has<PersistentCombatModifiers>()) {
        target.addComponent<PersistentCombatModifiers>();
      }
      PersistentCombatModifiers &persist =
          target.get<PersistentCombatModifiers>();
      persist.zingDelta += bonus.zingDelta;
      persist.bodyDelta += bonus.bodyDelta;
    }

    if (bonus.bonusEffects != nullptr && bonus.bonusEffectsCount > 0) {
      if (!target.has<SynergyBonusEffects>()) {
        target.addComponent<SynergyBonusEffects>();
      }
      SynergyBonusEffects &bonus_effects = target.get<SynergyBonusEffects>();
      for (size_t i = 0; i < bonus.bonusEffectsCount; ++i) {
        bonus_effects.effects.push_back(bonus.bonusEffects[i]);
      }
    }
  }
};
