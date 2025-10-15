#pragma once

#include "../dish_types.h"
#include <afterhours/ah.h>

enum struct GenerationTrigger {
  OnEnterCombat, // When dish enters combat phase
  OnFirstBite,   // On first attack/bite
  OnTakeDamage,  // When taking damage
  OnDealDamage,  // When dealing damage
  OnDefeat,      // When defeated
  OnVictory,     // When opponent defeated
  EveryTick,     // Every combat tick
  EveryNBites    // Every N bites
};

enum struct GenerationPosition {
  EndOfQueue,   // Add to end of team queue
  InSitu,       // Replace current dish in same slot
  FrontOfQueue, // Add to front of team queue
  AdjacentSlots // Fill adjacent empty slots
};

struct DishGenerationAbility : afterhours::BaseComponent {
  DishType generatedDishType = DishType::Potato;
  GenerationTrigger trigger = GenerationTrigger::OnEnterCombat;
  GenerationPosition position = GenerationPosition::EndOfQueue;

  // Timing/condition parameters
  int everyNBites = 1;        // For EveryNBites trigger
  int maxGenerations = 1;     // Max times this ability can trigger
  int currentGenerations = 0; // How many times triggered

  // Delay parameters
  float delaySeconds = 0.0f;    // Delay before generation
  float cooldownSeconds = 0.0f; // Cooldown between generations

  // Level scaling
  bool scalesWithLevel = true; // Generated dish inherits level
  int levelBonus = 0;          // Additional level bonus

  // Generation conditions
  bool requiresEmptySlot = false; // Only generate if target slot is empty
  bool replacesSelf = false;      // For InSitu: replace the generating dish
};
