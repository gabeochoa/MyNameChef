#pragma once

#include "../components/dish_battle_state.h"
#include "../components/is_dish.h"
#include "../game_state_manager.h"
#include "../log.h"
#include "../query.h"
#include <afterhours/ah.h>
#include <magic_enum/magic_enum.hpp>
#include <map>
#include <set>
#include <vector>

struct TestStateInspector : afterhours::System<> {
  static inline TestStateInspector *instance = nullptr;

  bool enabled = false;
  std::set<std::string> enabled_flags;

  struct ComponentSnapshot {
    std::string component_name;
    std::string data;
  };

  struct EntitySnapshot {
    afterhours::EntityID id;
    std::vector<ComponentSnapshot> components;
  };

  struct StateSnapshot {
    float timestamp;
    std::vector<EntitySnapshot> entities;
    std::map<std::string, std::string> singletons;
  };

  std::vector<StateSnapshot> history;
  float current_time = 0.0f;

  TestStateInspector() { instance = this; }

  ~TestStateInspector() {
    if (instance == this) {
      instance = nullptr;
    }
  }

  virtual bool should_run(float) override { return enabled; }

  void for_each_with(afterhours::Entity &, float dt) override {
    if (!enabled) {
      return;
    }

    current_time += dt;
    StateSnapshot snapshot;
    snapshot.timestamp = current_time;

    if (enabled_flags.count("entities") > 0) {
      capture_entities(snapshot);
    }

    if (enabled_flags.count("singletons") > 0) {
      capture_singletons(snapshot);
    }

    history.push_back(snapshot);
  }

  void enable(const std::set<std::string> &flags = {"entities"}) {
    enabled = true;
    enabled_flags = flags;
    history.clear();
    current_time = 0.0f;
  }

  void disable() { enabled = false; }

  void clear_history() {
    history.clear();
    current_time = 0.0f;
  }

  const std::vector<StateSnapshot> &get_history() const { return history; }

  static TestStateInspector *get_instance() { return instance; }

private:
  void capture_entities(StateSnapshot &snapshot) {
    for (afterhours::Entity &entity :
         afterhours::EntityQuery().whereHasComponent<IsDish>().gen()) {
      EntitySnapshot entity_snap;
      entity_snap.id = entity.id;

      if (entity.has<DishBattleState>()) {
        ComponentSnapshot comp_snap;
        comp_snap.component_name = "DishBattleState";
        const DishBattleState &dbs = entity.get<DishBattleState>();
        comp_snap.data =
            "phase=" + std::string(magic_enum::enum_name(dbs.phase)) +
            " slot=" + std::to_string(dbs.queue_index) +
            " team=" + std::string(magic_enum::enum_name(dbs.team_side));
        entity_snap.components.push_back(comp_snap);
      }

      snapshot.entities.push_back(entity_snap);
    }
  }

  void capture_singletons(StateSnapshot &snapshot) {
    auto &gsm = GameStateManager::get();
    snapshot.singletons["screen"] =
        std::string(magic_enum::enum_name(gsm.active_screen));
  }
};
