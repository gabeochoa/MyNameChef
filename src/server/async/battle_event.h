#pragma once

#include "../../components/dish_battle_state.h"
#include "../../components/trigger_event.h"

namespace server::async {
struct DebugBattleEvent {
  TriggerHook hook;
  int sourceEntityId;
  int slotIndex;
  DishBattleState::TeamSide teamSide;
  float timestamp;
  int courseIndex;
  int payloadInt;
  float payloadFloat;
};
} // namespace server::async
