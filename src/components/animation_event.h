#pragma once

#include <afterhours/ah.h>

// Marker for an animation event entity that should block battle progression
struct IsBlockingAnimationEvent : afterhours::BaseComponent {};

// Generic animation event payload; different kinds are identified by type
enum struct AnimationEventType { SlideIn, StatBoost };

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
