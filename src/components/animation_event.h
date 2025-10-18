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

// Marker to indicate the animation has been scheduled so we don't reschedule
// it every frame (which would prevent completion callbacks from firing)
struct AnimationEventScheduled : afterhours::BaseComponent {};

// Simple timer component for animations that don't use the complex animation
// system
struct AnimationTimer : afterhours::BaseComponent {
  float duration = 0.0f;
  float elapsed = 0.0f;
};

// Unified animation data component - contains all animation-specific data
struct AnimationData : afterhours::BaseComponent {
  // StatBoost data
  int targetEntityId = -1; // which dish gets the stat boost
  int zingDelta = 0;       // how much zing is being added
  int bodyDelta = 0;       // how much body is being added

  // FreshnessChain data
  int sourceEntityId = -1;   // the salmon dish that triggered the chain
  int previousEntityId = -1; // previous dish that gets boosted (if any)
  int nextEntityId = -1;     // next dish that gets boosted (if any)
};

// Legacy type aliases for backwards compatibility
using StatBoostAnimation = AnimationData;
using FreshnessChainAnimation = AnimationData;
