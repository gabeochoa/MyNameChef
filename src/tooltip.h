#pragma once

#include "dish_types.h"
#include <sstream>
#include <string>

inline std::string generate_dish_tooltip(DishType dishType) {
  auto dishInfo = get_dish_info(dishType);
  std::ostringstream tooltip;

  tooltip << "[GOLD]" << dishInfo.name << "\n";
  tooltip << "[CYAN]Price: " << dishInfo.price << " coins\n";
  tooltip << "Flavor Stats:\n";
  if (dishInfo.flavor.satiety > 0)
    tooltip << "[PURPLE]  Satiety: " << dishInfo.flavor.satiety << "\n";
  if (dishInfo.flavor.sweetness > 0)
    tooltip << "[YELLOW]  Sweetness: " << dishInfo.flavor.sweetness << "\n";
  if (dishInfo.flavor.spice > 0)
    tooltip << "[RED]  Spice: " << dishInfo.flavor.spice << "\n";
  if (dishInfo.flavor.acidity > 0)
    tooltip << "[GREEN]  Acidity: " << dishInfo.flavor.acidity << "\n";
  if (dishInfo.flavor.umami > 0)
    tooltip << "[BLUE]  Umami: " << dishInfo.flavor.umami << "\n";
  if (dishInfo.flavor.richness > 0)
    tooltip << "[ORANGE]  Richness: " << dishInfo.flavor.richness << "\n";
  if (dishInfo.flavor.freshness > 0)
    tooltip << "[CYAN]  Freshness: " << dishInfo.flavor.freshness << "\n";

  return tooltip.str();
}
