#pragma once

#include "../components/battle_session_registry.h"
#include "../components/battle_session_tag.h"
#include "../components/battle_team_data.h"
#include "../components/battle_team_tags.h"
#include "../components/combat_queue.h"
#include "../components/trigger_queue.h"
#include "../game_state_manager.h"
#include <afterhours/ah.h>

struct BattleTeamLoaderSystem : afterhours::System<> {
  bool tagged = false;
  GameStateManager::Screen last_screen = GameStateManager::Screen::Main;

  virtual bool should_run(float) override {
    auto &gsm = GameStateManager::get();

    if (last_screen == GameStateManager::Screen::Battle &&
        gsm.active_screen != GameStateManager::Screen::Battle) {
      tagged = false;
    }

    last_screen = gsm.active_screen;

    auto manager_entity =
        afterhours::EntityHelper::get_singleton<CombatQueue>();
    if (!manager_entity.get().has<CombatQueue>()) {
      return false;
    }

    bool player_instantiated = false;
    bool opponent_instantiated = false;

    if (manager_entity.get().has<BattleTeamDataPlayer>()) {
      const auto &data = manager_entity.get().get<BattleTeamDataPlayer>();
      player_instantiated = data.instantiated;
    }

    if (manager_entity.get().has<BattleTeamDataOpponent>()) {
      const auto &data = manager_entity.get().get<BattleTeamDataOpponent>();
      opponent_instantiated = data.instantiated;
    }

    return gsm.active_screen == GameStateManager::Screen::Battle && !tagged &&
           player_instantiated && opponent_instantiated;
  }

  void once(float) override {
    tag_battle_entities();
    tagged = true;
  }

private:
  void tag_battle_entities() {
    auto registry =
        afterhours::EntityHelper::get_singleton<BattleSessionRegistry>();
    if (!registry.get().has<BattleSessionRegistry>()) {
      return;
    }

    BattleSessionRegistry &reg = registry.get().get<BattleSessionRegistry>();
    reg.sessionId++;

    for (afterhours::Entity &e : afterhours::EntityQuery({.force_merge = true})
                                     .whereHasComponent<IsPlayerTeamItem>()
                                     .gen()) {
      e.addComponent<BattleSessionTag>(reg.sessionId);
      reg.ownedEntityIds.push_back(e.id);
    }

    for (afterhours::Entity &e : afterhours::EntityQuery({.force_merge = true})
                                     .whereHasComponent<IsOpponentTeamItem>()
                                     .gen()) {
      e.addComponent<BattleSessionTag>(reg.sessionId);
      reg.ownedEntityIds.push_back(e.id);
    }

    auto tq = afterhours::EntityHelper::get_singleton<TriggerQueue>();
    if (tq.get().has<TriggerQueue>()) {
      tq.get().addComponent<BattleSessionTag>(reg.sessionId);
      reg.ownedEntityIds.push_back(tq.get().id);
    }
  }
};
