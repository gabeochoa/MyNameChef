#pragma once

#include "../../../components/combat_queue.h"
#include "../../../components/trigger_event.h"
#include "../../../components/trigger_queue.h"
#include "../battle_event.h"
#include "../components/battle_info.h"
#include "../types.h"
#include <afterhours/ah.h>
#include <unordered_map>
#include <vector>

namespace server::async {
struct DebugServerEventLoggerSystem : afterhours::System<BattleInfo> {
  std::unordered_map<BattleId, std::vector<DebugBattleEvent>> event_logs;
  std::unordered_map<BattleId, float> battle_timestamps;

  void for_each_with(afterhours::Entity &battle_entity, BattleInfo &info,
                     float dt) override {
    if (info.status != BattleStatus::Running) {
      return;
    }

    // Track simulation time for this battle
    battle_timestamps[info.battleId] += dt;

    // Get course index from CombatQueue
    int course_index = 0;
    auto cq_entity = afterhours::EntityHelper::get_singleton<CombatQueue>();
    if (cq_entity.get().has<CombatQueue>()) {
      const CombatQueue &cq = cq_entity.get().get<CombatQueue>();
      course_index = cq.current_index;
    }

    auto tq_entity = afterhours::EntityHelper::get_singleton<TriggerQueue>();
    if (!tq_entity.get().has<TriggerQueue>()) {
      return;
    }

    const TriggerQueue &queue = tq_entity.get().get<TriggerQueue>();
    float timestamp = battle_timestamps[info.battleId];
    for (const TriggerEvent &ev : queue.events) {
      DebugBattleEvent battle_event;
      battle_event.hook = ev.hook;
      battle_event.sourceEntityId = ev.sourceEntityId;
      battle_event.slotIndex = ev.slotIndex;
      battle_event.teamSide = ev.teamSide;
      battle_event.timestamp = timestamp;
      battle_event.courseIndex = course_index;
      battle_event.payloadInt = ev.payloadInt;
      battle_event.payloadFloat = ev.payloadFloat;

      event_logs[info.battleId].push_back(battle_event);
    }
  }

  std::vector<DebugBattleEvent> get_events(const BattleId &battle_id) const {
    if (event_logs.find(battle_id) != event_logs.end()) {
      return event_logs.at(battle_id);
    }
    return {};
  }

  void clear_events(const BattleId &battle_id) {
    event_logs.erase(battle_id);
    battle_timestamps.erase(battle_id);
  }
};
} // namespace server::async
