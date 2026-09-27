#pragma once

#include <afterhours/ah.h>
#include <algorithm>

// Silver Cloche house theme — the chosen direction from game_mock.html:
// frameless GBA x DS two-tone, palette No.3 Quintonil (Modern Mexican).
// The mock's two pages are the palette background mixed toward white
// (left page) and toward the primary green (right page); the seam sits
// at the horizontal center. Cards are two-tone too: panel over a
// primary-tinted base (see RenderSystemHelpers / world rendering).
namespace cloche_theme {

// Quintonil palette (mock CSS variables, converted to RGB).
inline constexpr afterhours::Color PAGE_LEFT{240, 242, 232, 255};
inline constexpr afterhours::Color PAGE_RIGHT{190, 204, 168, 255};
inline constexpr afterhours::Color SEAM{34, 48, 26, 64};
inline constexpr afterhours::Color PANEL{255, 255, 255, 255};
inline constexpr afterhours::Color EDGE{157, 179, 138, 255};
inline constexpr afterhours::Color PRIMARY{90, 125, 42, 255};
inline constexpr afterhours::Color PRIMARY_HI{109, 148, 52, 255};
inline constexpr afterhours::Color SECONDARY{123, 162, 63, 255};
inline constexpr afterhours::Color ACCENT{193, 68, 14, 255};
inline constexpr afterhours::Color FONT{34, 48, 26, 255};
inline constexpr afterhours::Color FONT_DIM{103, 117, 90, 255};
inline constexpr afterhours::Color FONT_ON_DARK{255, 255, 255, 255};
inline constexpr afterhours::Color PRICE{138, 90, 0, 255};
inline constexpr afterhours::Color SILVER{199, 204, 209, 255};

// In-fiction screen copy (Silver Cloche / Aldercroft Guide).
inline constexpr const char *GAME_TITLE = "MY NAME CHEF";
inline constexpr const char *GAME_TAGLINE =
    "Every service is an audition for the Silver Cloche: lent, never owned.";

// Battle rows: both full starting teams always fit on screen. Dishes
// keep their size; the row is centered and the spacing shrinks as the
// team grows. (Fighters beyond a starting team are a spawn concern,
// not a layout one — they may enter from off-screen.)
inline constexpr float BATTLE_DISH_SIZE = 80.f;
inline constexpr float BATTLE_ROW_MAX_SPACING = 100.f;
inline constexpr float BATTLE_ROW_EDGE = 40.f;

struct BattleRowLayout {
  float start_x;
  float spacing;
};

inline BattleRowLayout battle_row_layout(int count, float row_width) {
  if (count <= 1) {
    return BattleRowLayout{(row_width - BATTLE_DISH_SIZE) / 2.f, 0.f};
  }
  float max_span = row_width - 2.f * BATTLE_ROW_EDGE;
  float spacing =
      std::min(BATTLE_ROW_MAX_SPACING,
               (max_span - BATTLE_DISH_SIZE) / static_cast<float>(count - 1));
  float span = BATTLE_DISH_SIZE + spacing * static_cast<float>(count - 1);
  return BattleRowLayout{(row_width - span) / 2.f, spacing};
}

inline float battle_slot_x(const BattleRowLayout &layout, int slot) {
  return layout.start_x + layout.spacing * static_cast<float>(slot);
}

} // namespace cloche_theme
