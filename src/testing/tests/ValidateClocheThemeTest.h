#pragma once

#include "../../ui/cloche_theme.h"
#include "../test_macros.h"

// The battle layout rule from the chosen mock direction: both full
// starting teams always fit on screen, at any supported width, for any
// team size up to a full bench (INVENTORY_SLOTS = 7). Dishes never
// shrink; spacing does.
TEST(validate_cloche_battle_row_fit) {
  const float widths[] = {800.f, 1024.f, 1280.f, 1600.f, 1920.f};
  for (float width : widths) {
    for (int count = 1; count <= 7; ++count) {
      auto layout = cloche_theme::battle_row_layout(count, width);
      float first_x = cloche_theme::battle_slot_x(layout, 0);
      float last_x = cloche_theme::battle_slot_x(layout, count - 1);
      app.expect_true(first_x >= 0.f, "battle row starts on screen");
      app.expect_true(last_x + cloche_theme::BATTLE_DISH_SIZE <= width,
                      "battle row ends on screen");
      if (count > 1) {
        float expected_last =
            layout.start_x + layout.spacing * static_cast<float>(count - 1);
        app.expect_eq(last_x, expected_last, "slot x follows spacing");
        app.expect_true(layout.spacing > 0.f, "spacing stays positive");
        app.expect_true(layout.spacing <=
                            cloche_theme::BATTLE_ROW_MAX_SPACING,
                        "spacing never exceeds the comfortable maximum");
      }
    }
  }

  // A full bench at the default resolution keeps the classic spacing.
  auto full = cloche_theme::battle_row_layout(7, 1280.f);
  app.expect_eq(full.spacing, cloche_theme::BATTLE_ROW_MAX_SPACING,
                "full bench at 1280 keeps 100px spacing");
  // A single dish is centered.
  auto single = cloche_theme::battle_row_layout(1, 1280.f);
  app.expect_eq(single.start_x,
                (1280.f - cloche_theme::BATTLE_DISH_SIZE) / 2.f,
                "single dish is centered");
}

TEST(validate_cloche_palette_contrast) {
  auto luminance = [](const afterhours::Color &c) {
    return (0.2126f * c.r + 0.7152f * c.g + 0.0722f * c.b) / 255.f;
  };
  // The two pages must actually read as two tones.
  app.expect_true(luminance(cloche_theme::PAGE_LEFT) >
                      luminance(cloche_theme::PAGE_RIGHT),
                  "left page is lighter than right page");
  // Body text must be dark against both light pages.
  app.expect_true(luminance(cloche_theme::FONT) <
                      luminance(cloche_theme::PAGE_RIGHT),
                  "font is darker than both pages");
  app.expect_true(luminance(cloche_theme::PAGE_LEFT) -
                          luminance(cloche_theme::FONT) >
                      0.5f,
                  "font on left page has strong luminance contrast");
}
