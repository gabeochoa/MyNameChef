#pragma once

#include "dish_types.h"
#include <magic_enum/magic_enum.hpp>
#include <sstream>
#include <string>

static std::string format_target_scope(TargetScope scope) {
  switch (scope) {
  case TargetScope::Self:
    return "self";
  case TargetScope::Opponent:
    return "opponent";
  case TargetScope::AllAllies:
    return "all allies";
  case TargetScope::AllOpponents:
    return "all opponents";
  case TargetScope::DishesAfterSelf:
    return "dishes after self";
  case TargetScope::FutureAllies:
    return "future allies";
  case TargetScope::FutureOpponents:
    return "future opponents";
  case TargetScope::Previous:
    return "previous dish";
  case TargetScope::Next:
    return "next dish";
  case TargetScope::SelfAndAdjacent:
    return "self and adjacent";
  }
  return "unknown";
}

static std::string format_flavor_stat(FlavorStatType stat) {
  switch (stat) {
  case FlavorStatType::Satiety:
    return "Satiety";
  case FlavorStatType::Sweetness:
    return "Sweetness";
  case FlavorStatType::Spice:
    return "Spice";
  case FlavorStatType::Acidity:
    return "Acidity";
  case FlavorStatType::Umami:
    return "Umami";
  case FlavorStatType::Richness:
    return "Richness";
  case FlavorStatType::Freshness:
    return "Freshness";
  }
  return "Unknown";
}

static std::string format_effect_description(const DishEffect &effect) {
  std::ostringstream desc;
  std::string triggerName = std::string(magic_enum::enum_name(effect.triggerHook));
  
  desc << "[WHITE]" << triggerName << ": ";
  
  if (effect.conditional) {
    std::string checkStat = format_flavor_stat(effect.adjacentCheckStat);
    desc << "If adjacent has " << checkStat << ", ";
  }
  
  std::string target = format_target_scope(effect.targetScope);
  
  switch (effect.operation) {
  case EffectOperation::AddFlavorStat: {
    std::string statName = format_flavor_stat(effect.flavorStatType);
    std::string sign = effect.amount >= 0 ? "+" : "";
    desc << sign << effect.amount << " " << statName << " to " << target;
    break;
  }
  case EffectOperation::AddCombatZing: {
    std::string sign = effect.amount >= 0 ? "+" : "";
    desc << sign << effect.amount << " Zing to " << target;
    break;
  }
  case EffectOperation::AddCombatBody: {
    std::string sign = effect.amount >= 0 ? "+" : "";
    desc << sign << effect.amount << " Body to " << target;
    break;
  }
  }
  
  return desc.str();
}

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

  if (!dishInfo.effects.empty()) {
    tooltip << "\n[GOLD]Effects:\n";
    for (const auto &effect : dishInfo.effects) {
      tooltip << format_effect_description(effect) << "\n";
    }
  }

  return tooltip.str();
}

inline std::string generate_dish_tooltip_with_level(DishType dishType,
                                                    int level,
                                                    int merge_progress,
                                                    int merges_needed) {
  auto dishInfo = get_dish_info(dishType, level);
  std::ostringstream tooltip;

  tooltip << "[GOLD]" << dishInfo.name << "\n";
  tooltip << "[CYAN]Price: " << dishInfo.price << " coins\n";
  tooltip << "[YELLOW]Level: " << level << "\n";
  tooltip << "[WHITE]Progress: " << merge_progress << "/" << merges_needed
          << " merges\n";
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

  if (!dishInfo.effects.empty()) {
    tooltip << "\n[GOLD]Effects:\n";
    for (const auto &effect : dishInfo.effects) {
      tooltip << format_effect_description(effect) << "\n";
    }
  }

  return tooltip.str();
}
