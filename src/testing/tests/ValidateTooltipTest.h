#pragma once

#include "../../components/dish_level.h"
#include "../../dish_types.h"
#include "../../tooltip.h"
#include "../test_app.h"
#include "../test_macros.h"
#include <string>

namespace {

bool tooltip_contains(const std::string &text, const std::string &needle) {
  return text.find(needle) != std::string::npos;
}

} // namespace

TEST(validate_tooltip_level_preview_and_unlocks) {
  std::string potato_one =
      generate_dish_tooltip_with_level(DishType::Potato, 1, 0, 2);
  app.expect_true(
      tooltip_contains(potato_one, "Later Unlocks:"),
      "Potato level 1 shows later unlocks for its level 3 effect");
  app.expect_true(
      tooltip_contains(potato_one, "Level 3:"),
      "Potato level 1 previews the effect unlocking at level 3");
  app.expect_true(tooltip_contains(potato_one, "Satiety"),
                  "Potato level 1 unlock preview names Satiety");
  app.expect_false(
      tooltip_contains(potato_one, "Next Level:"),
      "Potato level 2 def matches level 1, so no next level preview");

  std::string potato_two =
      generate_dish_tooltip_with_level(DishType::Potato, 2, 0, 2);
  app.expect_true(tooltip_contains(potato_two, "Next Level:"),
                  "Potato level 2 shows next level preview");
  app.expect_true(tooltip_contains(potato_two, "[NEW] Level 3:"),
                  "Potato level 2 marks the level 3 effect as new");

  std::string bagel_one =
      generate_dish_tooltip_with_level(DishType::Bagel, 1, 0, 2);
  app.expect_true(tooltip_contains(bagel_one, "Level 2:"),
                  "Bagel level 1 previews the scaled level 2 effect");
  app.expect_true(tooltip_contains(bagel_one, "+2 Richness"),
                  "Bagel level 1 preview shows the scaled amount");
}

TEST(validate_tooltip_combat_stats) {
  std::string garlic =
      generate_dish_tooltip_with_level(DishType::GarlicBread, 1, 0, 2);
  app.expect_true(tooltip_contains(garlic, "Combat Stats:"),
                  "GarlicBread tooltip shows combat stats");
  app.expect_true(tooltip_contains(garlic, "Zing: 0, Body: 2"),
                  "GarlicBread combat stats match its flavor at level 1");

  std::string wagyu =
      generate_dish_tooltip_with_level(DishType::WagyuSteak, 2, 0, 2);
  app.expect_true(tooltip_contains(wagyu, "Zing: 6, Body: 10"),
                  "Wagyu combat stats double at level 2");
  app.expect_true(tooltip_contains(wagyu, "(base Zing: 3, Body: 5)"),
                  "Wagyu tooltip shows unscaled base combat stats");
}

TEST(validate_tooltip_effect_grouping_and_status) {
  std::string garlic =
      generate_dish_tooltip_with_level(DishType::GarlicBread, 1, 0, 2);
  app.expect_true(tooltip_contains(garlic, "[BOLD]OnServe[/BOLD]"),
                  "GarlicBread effects group under an OnServe header");
  app.expect_true(tooltip_contains(garlic, "+1 Spice to future allies"),
                  "GarlicBread spice buff listed in its group");
  app.expect_true(
      tooltip_contains(garlic, "Apply Fatigue (-1 Body) to opponent"),
      "Fatigue shows its type and Body delta");
  app.expect_true(tooltip_contains(garlic, "(2 courses)"),
                  "Fatigue shows its duration");
  app.expect_true(tooltip_contains(garlic, "If target has Fatigue:"),
                  "chained effect shows its status condition");
  app.expect_true(tooltip_contains(garlic, "-> Then:"),
                  "chained effect shown as a follow-up line");
}

TEST(validate_tooltip_max_level) {
  std::string potato_max = generate_dish_tooltip_with_level(
      DishType::Potato, MAX_DISH_LEVEL, 0, 2);
  app.expect_true(tooltip_contains(potato_max, "(Max Level)"),
                  "max level dish shows the Max Level marker");
  app.expect_false(tooltip_contains(potato_max, "Next Level:"),
                   "max level dish shows no next level preview");
  app.expect_false(tooltip_contains(potato_max, "Later Unlocks:"),
                   "max level dish shows no later unlocks");
  app.expect_false(tooltip_contains(potato_max, "(Level 11:"),
                   "max level dish shows no level 11 stat preview");
}

TEST(validate_tooltip_base_generator) {
  std::string potato = generate_dish_tooltip(DishType::Potato);
  app.expect_true(tooltip_contains(potato, "Potato"),
                  "base tooltip names the dish");
  app.expect_true(tooltip_contains(potato, "Combat Stats:"),
                  "base tooltip shows combat stats");
  app.expect_false(tooltip_contains(potato, "Next Level:"),
                   "base tooltip has no level preview");
}
