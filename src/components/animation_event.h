#pragma once

#include <afterhours/ah.h>

// Marker for an animation event entity that should block battle progression
struct IsBlockingAnimationEvent : afterhours::BaseComponent {};

// Generic animation event payload; different kinds are identified by type
enum struct AnimationEventType { SlideIn };

struct AnimationEvent : afterhours::BaseComponent {
  AnimationEventType type = AnimationEventType::SlideIn;
  int slotIndex = -1; // which course slot, if applicable
  int entityId = -1;  // optional source entity
};
