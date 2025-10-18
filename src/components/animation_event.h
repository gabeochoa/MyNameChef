#pragma once

#include <afterhours/ah.h>

// Marker for an animation event entity that should block battle progression
struct IsBlockingAnimationEvent : afterhours::BaseComponent {};

// Generic animation event payload; different kinds are identified by type
enum struct AnimationEventType { SlideIn, StatBoost, FreshnessChain };

struct AnimationEvent : afterhours::BaseComponent {
  AnimationEventType type = AnimationEventType::SlideIn;
  int slotIndex = -1; // which course slot, if applicable
  int entityId = -1;  // optional source entity
};

// Component for stat boost animations - tracks which dish gets the +1 overlay
struct StatBoostAnimation : afterhours::BaseComponent {
  int targetEntityId = -1; // which dish gets the stat boost
  int zingDelta = 0;       // how much zing is being added
  int bodyDelta = 0;       // how much body is being added
};

// Component for freshness chain animations - tracks which dishes get freshness
// boosts
struct FreshnessChainAnimation : afterhours::BaseComponent {
  int sourceEntityId = -1;   // the salmon dish that triggered the chain
  int previousEntityId = -1; // previous dish that gets boosted (if any)
  int nextEntityId = -1;     // next dish that gets boosted (if any)
};
