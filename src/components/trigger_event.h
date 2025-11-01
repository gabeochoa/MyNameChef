#pragma once

#include "../components/dish_battle_state.h"
#include <afterhours/ah.h>

// Trigger execution order during battle:
// 1. OnStartBattle - Fires once when battle begins (before any dishes serve)
// 2. OnServe - Fires for all dishes in slot order (left-to-right) before combat
// starts
//              This happens while dishes are still InQueue phase
// 3. OnCourseStart - Fires when a course begins (both dishes transition InQueue
// -> Entering)
// 4. OnBiteTaken - Fires each time damage is dealt during combat (alternating
// turns)
// 5. OnDishFinished - Fires when a dish's Body reaches 0 (defeated)
// 6. OnCourseComplete - Fires when both dishes in a course are Finished
//
// Note: OnServe fires for ALL dishes before any OnCourseStart occurs.
//       Then courses proceed sequentially, with OnCourseStart -> OnBiteTaken*
//       -> OnDishFinished -> OnCourseComplete for each course.

enum struct TriggerHook {
  OnStartBattle,   // Battle initialization (fires once at start)
  OnServe,         // Dish serves (fires for all dishes before combat)
  OnCourseStart,   // Course begins (both dishes entering combat)
  OnBiteTaken,     // Damage dealt (fires repeatedly during combat)
  OnDishFinished,  // Dish defeated (Body <= 0)
  OnCourseComplete // Course ends (both dishes finished)
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
