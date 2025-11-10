#pragma once

#include "render_backend.h"

struct BattleTiming {
  static constexpr float kTickMs = 150.0f / 1000.0f;
  static constexpr float kPrePauseMs = 0.35f;
  static constexpr float kPostPauseMs = 0.35f;
  static constexpr float kEnterDuration = 0.45f;
  static constexpr float kEnterStartDelay = 0.25f;
  static constexpr float kSlideInDuration = 0.27f;
  static constexpr float kStatBoostDuration = 1.5f;
  static constexpr float kFreshnessChainDuration = 2.0f;

  static float get_tick_duration() {
    return kTickMs / render_backend::timing_speed_scale;
  }

  static float get_pre_pause() {
    return kPrePauseMs / render_backend::timing_speed_scale;
  }

  static float get_post_pause() {
    return kPostPauseMs / render_backend::timing_speed_scale;
  }

  static float get_enter_duration() {
    return kEnterDuration / render_backend::timing_speed_scale;
  }

  static float get_enter_start_delay() {
    return kEnterStartDelay / render_backend::timing_speed_scale;
  }

  static float get_slide_in_duration() {
    return kSlideInDuration / render_backend::timing_speed_scale;
  }

  static float get_stat_boost_duration() {
    return kStatBoostDuration / render_backend::timing_speed_scale;
  }

  static float get_freshness_chain_duration() {
    return kFreshnessChainDuration / render_backend::timing_speed_scale;
  }
};

