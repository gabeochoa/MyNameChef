#pragma once

#include <afterhours/ah.h>
#include "../sound_library.h"

struct PlaySoundRequest : afterhours::BaseComponent {
  enum struct Policy {
    Enum,                 // Play specific sound file
    Name,                 // Play sound by name
    PrefixFirstAvailable, // Play first available sound with prefix
    PrefixIfNonePlaying,  // Play prefix sound only if none are playing
    PrefixRandom          // Play random sound with prefix
  };

  Policy policy = Policy::Enum;
  SoundFile file = SoundFile::UI_Select;
  std::string name;
  std::string prefix;
  bool prefer_alias = true;

  PlaySoundRequest() = default;
  explicit PlaySoundRequest(SoundFile f)
      : policy(Policy::Enum), file(f), prefer_alias(true) {}
};