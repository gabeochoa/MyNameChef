#pragma once

#include <afterhours/ah.h>

namespace afterhours::ui::metrics {

inline auto h720(float px) { return afterhours::ui::screen_pct(px / 720.f); }
inline auto w1280(float px) { return afterhours::ui::screen_pct(px / 1280.f); }

} // namespace afterhours::ui::metrics
