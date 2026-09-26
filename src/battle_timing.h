#pragma once

#include "render_backend.h"

// Issue 65: single timing scale - the main loop never scales dt; these
// durations are the only place timing_speed_scale applies, so a requested
// scale S makes combat run S times faster (tests use 5, any value works).
struct BattleTiming {
  static constexpr float TICK_MS = 150.0f / 1000.0f;
  static constexpr float PRE_PAUSE_MS = 0.35f;
  static constexpr float POST_PAUSE_MS = 0.35f;
  static constexpr float ENTER_DURATION = 0.45f;
  static constexpr float ENTER_START_DELAY = 0.25f;
  static constexpr float SLIDE_IN_DURATION = 0.27f;
  static constexpr float STAT_BOOST_DURATION = 1.5f;
  static constexpr float FRESHNESS_CHAIN_DURATION = 2.0f;

  static float get_tick_duration() { return TICK_MS / render_backend::timing_speed_scale; }
  static float get_pre_pause() { return PRE_PAUSE_MS / render_backend::timing_speed_scale; }
  static float get_post_pause() { return POST_PAUSE_MS / render_backend::timing_speed_scale; }
  static float get_enter_duration() { return ENTER_DURATION / render_backend::timing_speed_scale; }
  static float get_enter_start_delay() { return ENTER_START_DELAY / render_backend::timing_speed_scale; }
  static float get_slide_in_duration() { return SLIDE_IN_DURATION / render_backend::timing_speed_scale; }
  static float get_stat_boost_duration() { return STAT_BOOST_DURATION / render_backend::timing_speed_scale; }
  static float get_freshness_chain_duration() { return FRESHNESS_CHAIN_DURATION / render_backend::timing_speed_scale; }
};
