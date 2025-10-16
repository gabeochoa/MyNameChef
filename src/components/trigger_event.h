#pragma once

#include "../components/dish_battle_state.h"
#include <afterhours/ah.h>

enum struct TriggerHook {
  OnServe,
  OnBiteTaken,
  OnDishFinished,
  OnCourseStart,
  OnCourseComplete
};

struct TriggerEvent : afterhours::BaseComponent {
  TriggerHook hook = TriggerHook::OnServe;
  int sourceEntityId = 0;
  int slotIndex = 0;
  DishBattleState::TeamSide teamSide = DishBattleState::TeamSide::Player;

  // Payload data (can be extended as needed)
  int payloadInt = 0;
  float payloadFloat = 0.0f;

  TriggerEvent() = default;
  TriggerEvent(TriggerHook h, int source, int slot,
               DishBattleState::TeamSide side)
      : hook(h), sourceEntityId(source), slotIndex(slot), teamSide(side) {}
};
