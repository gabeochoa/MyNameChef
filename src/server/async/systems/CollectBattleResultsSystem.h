#pragma once

#include "../../../components/dish_battle_state.h"
#include "../../../log.h"
#include "../../battle_serializer.h"
#include "../battle_event.h"
#include "../components/battle_info.h"
#include "../components/event_log.h"
#include <afterhours/ah.h>
#include <magic_enum/magic_enum.hpp>

namespace server::async {
struct CollectBattleResultsSystem : afterhours::System<BattleInfo> {
  void for_each_with(afterhours::Entity &battle_entity, BattleInfo &info,
                     float) override {
    // Only process battles that are complete but don't have results yet
    if (info.status != BattleStatus::Complete || !info.result.empty()) {
      return;
    }

    nlohmann::json outcomes =
        server::BattleSerializer::collect_battle_outcomes();
    std::vector<DebugBattleEvent> events;

    // Find EventLog entity for this battle
    auto event_log_opt = afterhours::EntityQuery({.force_merge = true})
                             .whereHasComponent<EventLog>()
                             .whereLambda([&info](const afterhours::Entity &e) {
                               const EventLog &log = e.get<EventLog>();
                               return log.battleId == info.battleId;
                             })
                             .gen_first();

    if (event_log_opt) {
      const EventLog &event_log = event_log_opt.asE().get<EventLog>();
      events = event_log.events;
    }

    nlohmann::json events_json = nlohmann::json::array();
    for (const auto &ev : events) {
      nlohmann::json event_json;
      event_json["hook"] = std::string(magic_enum::enum_name(ev.hook));
      event_json["sourceEntityId"] = ev.sourceEntityId;
      event_json["slotIndex"] = ev.slotIndex;
      std::string team_side_str;
      switch (ev.teamSide) {
      case DishBattleState::TeamSide::Player:
        team_side_str = "Player";
        break;
      case DishBattleState::TeamSide::Opponent:
        team_side_str = "Opponent";
        break;
      }
      event_json["teamSide"] = team_side_str;
      event_json["timestamp"] = ev.timestamp;
      event_json["courseIndex"] = ev.courseIndex;
      event_json["payloadInt"] = ev.payloadInt;
      event_json["payloadFloat"] = ev.payloadFloat;
      events_json.push_back(event_json);
    }

    nlohmann::json result = server::BattleSerializer::serialize_battle_result(
        info.seed, info.opponentId, outcomes, events_json, false);

    info.result = result;
    log_info("SERVER_BATTLE: Collected results for battle {}", info.battleId);

    // Clean up EventLog entity
    if (event_log_opt) {
      event_log_opt.asE().cleanup = true;
    }
  }
};
} // namespace server::async
