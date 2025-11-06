#pragma once

#include "../battle_event.h"
#include "../components/battle_info.h"
#include "../components/event_log.h"
#include "../types.h"
#include "../../../components/combat_queue.h"
#include "../../../components/trigger_event.h"
#include "../../../components/trigger_queue.h"
#include <afterhours/ah.h>

namespace server::async {
struct DebugServerEventLoggerSystem : afterhours::System<BattleInfo> {
  void for_each_with(afterhours::Entity &battle_entity, BattleInfo &info,
                     float dt) override {
    if (info.status != BattleStatus::Running) {
      return;
    }

    // Find or create EventLog entity for this battle
    auto event_log_opt = afterhours::EntityQuery({.force_merge = true})
                             .whereHasComponent<EventLog>()
                             .whereLambda([&info](const afterhours::Entity &e) {
                               const EventLog &log = e.get<EventLog>();
                               return log.battleId == info.battleId;
                             })
                             .gen_first();

    afterhours::Entity *event_log_entity = nullptr;
    if (event_log_opt) {
      event_log_entity = &event_log_opt.asE();
    } else {
      auto &new_entity = afterhours::EntityHelper::createEntity();
      auto &event_log = new_entity.addComponent<EventLog>();
      event_log.battleId = info.battleId;
      event_log.timestamp = 0.0f;
      event_log_entity = &new_entity;
    }

    EventLog &event_log = event_log_entity->get<EventLog>();
    event_log.timestamp += dt;

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
    float timestamp = event_log.timestamp;
    for (const TriggerEvent &ev : queue.events) {
      DebugBattleEvent battle_event = {.hook = ev.hook,
                                       .sourceEntityId = ev.sourceEntityId,
                                       .slotIndex = ev.slotIndex,
                                       .teamSide = ev.teamSide,
                                       .timestamp = timestamp,
                                       .courseIndex = course_index,
                                       .payloadInt = ev.payloadInt,
                                       .payloadFloat = ev.payloadFloat};

      event_log.events.push_back(battle_event);
    }
  }
};
} // namespace server::async

