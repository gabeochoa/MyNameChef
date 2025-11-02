#pragma once

#include <afterhours/ah.h>
#include <string>

struct ReplayState : afterhours::BaseComponent {
  bool active = true; // Always active during battles for replay functionality
  bool paused = false;
  float timeScale = 1.0f;
  int64_t clockMs = 0;
  int64_t targetMs = 0;
  int64_t currentFrame = 0; // Current frame in the replay
  int64_t totalFrames = 0;  // Total length of simulation in frames
  uint64_t seed = 0;
  int totalCourses = 7;
  std::string playerJsonPath;
  std::string opponentJsonPath;

  ReplayState() = default;
};
