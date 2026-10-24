#pragma once

#include "components/dish_level.h"
#include "dish_types.h"
#include <algorithm>
#include <magic_enum/magic_enum.hpp>
#include <sstream>
#include <string>
#include <vector>

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
  case TargetScope::RandomAlly:
    return "random ally";
  case TargetScope::RandomOpponent:
    return "random opponent";
  case TargetScope::RandomDish:
    return "random dish";
  case TargetScope::RandomOtherAlly:
    return "random other ally";
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

static std::string format_effect_description(const DishEffect &effect,
                                             bool include_trigger = true,
                                             int depth = 0) {
  std::ostringstream desc;
  desc << "[COLOR:Text]";
  if (include_trigger) {
    desc << std::string(magic_enum::enum_name(effect.triggerHook)) << ": ";
  }

  if (effect.requiresStatus.has_value()) {
    desc << "If target has "
         << std::string(magic_enum::enum_name(*effect.requiresStatus))
         << ": ";
  }
  if (effect.conditional) {
    std::string checkStat = format_flavor_stat(effect.adjacentCheckStat);
    desc << "If adjacent has " << checkStat << ": ";
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
  case EffectOperation::SwapStats: {
    desc << "Swap Zing and Body for " << target;
    break;
  }
  case EffectOperation::MultiplyDamage: {
    float multiplier =
        effect.amount > 0 ? static_cast<float>(effect.amount) : 2.0f;
    desc << "Next attack deals " << multiplier << "x damage to " << target;
    break;
  }
  case EffectOperation::PreventAllDamage: {
    int count = effect.amount > 0 ? effect.amount : 1;
    desc << "Prevent next " << count << " damage to " << target;
    break;
  }
  case EffectOperation::CopyEffect: {
    desc << "Copy all effects from " << target;
    break;
  }
  case EffectOperation::SummonDish: {
    if (effect.summonDishType.has_value()) {
      auto dish_info = get_dish_info(effect.summonDishType.value());
      desc << "Summon " << dish_info.name << " to " << target;
    } else {
      desc << "Summon dish to " << target;
    }
    break;
  }
  case EffectOperation::ApplyStatus: {
    desc << "Apply "
         << std::string(magic_enum::enum_name(effect.statusType));
    std::string deltas;
    if (effect.amount != 0) {
      std::string sign = effect.amount >= 0 ? "+" : "";
      deltas += sign + std::to_string(effect.amount) + " Zing";
    }
    if (effect.statusBodyDelta != 0) {
      if (!deltas.empty()) {
        deltas += ", ";
      }
      std::string sign = effect.statusBodyDelta >= 0 ? "+" : "";
      deltas += sign + std::to_string(effect.statusBodyDelta) + " Body";
    }
    if (!deltas.empty()) {
      desc << " (" << deltas << ")";
    }
    desc << " to " << target;
    if (effect.duration > 0) {
      desc << " (" << effect.duration << " courses)";
    }
    break;
  }
  }

  if (!effect.chainedEffects.empty()) {
    if (depth >= MAX_EFFECT_CHAIN_DEPTH) {
      desc << "\n  [CHAIN TOO DEEP]";
    } else {
      for (const auto &chained : effect.chainedEffects) {
        desc << "\n  -> Then: "
             << format_effect_description(chained, false, depth + 1);
      }
    }
  }

  return desc.str();
}

static std::string format_effects_section(
    const std::vector<DishEffect> &effects) {
  std::ostringstream out;
  std::vector<TriggerHook> hooks;
  for (const auto &effect : effects) {
    if (std::find(hooks.begin(), hooks.end(), effect.triggerHook) ==
        hooks.end()) {
      hooks.push_back(effect.triggerHook);
    }
  }
  for (TriggerHook hook : hooks) {
    out << "[BOLD]" << std::string(magic_enum::enum_name(hook)) << "[/BOLD]\n";
    for (const auto &effect : effects) {
      if (effect.triggerHook == hook) {
        out << "  " << format_effect_description(effect, false) << "\n";
      }
    }
  }
  return out.str();
}

static bool effects_are_similar(const DishEffect &a, const DishEffect &b) {
  return a.triggerHook == b.triggerHook &&
         a.targetScope == b.targetScope && a.operation == b.operation;
}

static bool effects_differ(const DishEffect &a, const DishEffect &b) {
  return a.amount != b.amount ||
         a.statusBodyDelta != b.statusBodyDelta ||
         a.duration != b.duration ||
         a.flavorStatType != b.flavorStatType ||
         a.requiresStatus != b.requiresStatus ||
         a.conditional != b.conditional ||
         a.chainedEffects.size() != b.chainedEffects.size();
}

static std::string format_next_level_effects(
    const std::vector<DishEffect> &current,
    const std::vector<DishEffect> &next, int nextLevel) {
  std::ostringstream out;
  for (const auto &nextEffect : next) {
    bool found = false;
    bool differs = false;
    for (const auto &currentEffect : current) {
      if (effects_are_similar(currentEffect, nextEffect)) {
        found = true;
        differs = effects_differ(currentEffect, nextEffect);
        break;
      }
    }
    if (!found) {
      out << "[COLOR:Success][NEW] Level " << nextLevel << ": "
          << format_effect_description(nextEffect, true) << "\n";
    } else if (differs) {
      out << "[COLOR:TextMuted]Level " << nextLevel << ": "
          << format_effect_description(nextEffect, true) << "\n";
    }
  }
  return out.str();
}

static std::string format_later_unlocks(DishType dishType, int level) {
  std::ostringstream out;
  std::vector<DishEffect> emitted;
  for (int laterLevel = level + 2; laterLevel <= MAX_DISH_LEVEL;
       ++laterLevel) {
    const auto &laterEffects = get_dish_info(dishType, laterLevel).effects;
    for (const auto &effect : laterEffects) {
      bool known = false;
      for (int earlierLevel = level; earlierLevel < laterLevel;
           ++earlierLevel) {
        const auto &earlierEffects =
            get_dish_info(dishType, earlierLevel).effects;
        for (const auto &earlier : earlierEffects) {
          if (effects_are_similar(earlier, effect)) {
            known = true;
            break;
          }
        }
        if (known) {
          break;
        }
      }
      if (known) {
        continue;
      }
      for (const auto &already : emitted) {
        if (effects_are_similar(already, effect)) {
          known = true;
          break;
        }
      }
      if (known) {
        continue;
      }
      emitted.push_back(effect);
      out << "[COLOR:TextMuted]Level " << laterLevel << ": "
          << format_effect_description(effect, true) << "\n";
    }
  }
  return out.str();
}

static void append_flavor_stat_line(std::ostringstream &tooltip,
                                    const char *color, const char *name,
                                    int current, bool showNext, int next,
                                    int nextLevel) {
  if (current <= 0 && (!showNext || next <= 0)) {
    return;
  }
  tooltip << color << "  " << name << ": " << current;
  if (showNext && next != current) {
    tooltip << " (Level " << nextLevel << ": " << next << ")";
  }
  tooltip << "\n";
}

static void append_combat_stats_line(std::ostringstream &tooltip,
                                     const FlavorStats &flavor, int level) {
  int clampedLevel = std::clamp(level, 1, MAX_DISH_LEVEL);
  int scale = 1 << (clampedLevel - 1);
  int baseZing = flavor.zing();
  int baseBody = flavor.body();
  tooltip << "[COLOR:Info]Combat Stats: [COLOR:Text]Zing: "
          << baseZing * scale << ", Body: " << baseBody * scale;
  if (scale > 1) {
    tooltip << " (base Zing: " << baseZing << ", Body: " << baseBody << ")";
  }
  tooltip << "\n";
}

inline std::string generate_dish_tooltip(DishType dishType) {
  auto dishInfo = get_dish_info(dishType);
  std::ostringstream tooltip;

  tooltip << "[COLOR:Gold]" << dishInfo.name << "\n";
  tooltip << "[COLOR:Info]Price: " << dishInfo.price << " coins\n";
  tooltip << "Flavor Stats:\n";
  append_flavor_stat_line(tooltip, "[COLOR:Accent]", "Satiety",
                          dishInfo.flavor.satiety, false, 0, 0);
  append_flavor_stat_line(tooltip, "[COLOR:Warning]", "Sweetness",
                          dishInfo.flavor.sweetness, false, 0, 0);
  append_flavor_stat_line(tooltip, "[COLOR:Error]", "Spice",
                          dishInfo.flavor.spice, false, 0, 0);
  append_flavor_stat_line(tooltip, "[COLOR:Success]", "Acidity",
                          dishInfo.flavor.acidity, false, 0, 0);
  append_flavor_stat_line(tooltip, "[COLOR:Info]", "Umami",
                          dishInfo.flavor.umami, false, 0, 0);
  append_flavor_stat_line(tooltip, "[COLOR:Warning]", "Richness",
                          dishInfo.flavor.richness, false, 0, 0);
  append_flavor_stat_line(tooltip, "[COLOR:Info]", "Freshness",
                          dishInfo.flavor.freshness, false, 0, 0);
  append_combat_stats_line(tooltip, dishInfo.flavor, 1);

  if (!dishInfo.effects.empty()) {
    tooltip << "\n[COLOR:Gold]Effects:\n";
    tooltip << format_effects_section(dishInfo.effects);
  }

  return tooltip.str();
}

inline std::string generate_dish_tooltip_with_level(DishType dishType,
                                                    int level,
                                                    int merge_progress,
                                                    int merges_needed) {
  auto dishInfo = get_dish_info(dishType, level);
  bool atMaxLevel = level >= MAX_DISH_LEVEL;
  bool showNext = !atMaxLevel;
  auto nextInfo = showNext ? get_dish_info(dishType, level + 1) : dishInfo;
  std::ostringstream tooltip;

  tooltip << "[COLOR:Gold]" << dishInfo.name << "\n";
  tooltip << "[COLOR:Info]Price: " << dishInfo.price << " coins\n";
  tooltip << "[COLOR:Warning]Level: " << level;
  if (atMaxLevel) {
    tooltip << " [COLOR:Warning](Max Level)";
  }
  tooltip << "\n";
  tooltip << "[COLOR:Text]Progress: " << merge_progress << "/" << merges_needed
          << " merges\n";
  tooltip << "Flavor Stats:\n";
  append_flavor_stat_line(tooltip, "[COLOR:Accent]", "Satiety",
                          dishInfo.flavor.satiety, showNext,
                          nextInfo.flavor.satiety, level + 1);
  append_flavor_stat_line(tooltip, "[COLOR:Warning]", "Sweetness",
                          dishInfo.flavor.sweetness, showNext,
                          nextInfo.flavor.sweetness, level + 1);
  append_flavor_stat_line(tooltip, "[COLOR:Error]", "Spice",
                          dishInfo.flavor.spice, showNext,
                          nextInfo.flavor.spice, level + 1);
  append_flavor_stat_line(tooltip, "[COLOR:Success]", "Acidity",
                          dishInfo.flavor.acidity, showNext,
                          nextInfo.flavor.acidity, level + 1);
  append_flavor_stat_line(tooltip, "[COLOR:Info]", "Umami",
                          dishInfo.flavor.umami, showNext,
                          nextInfo.flavor.umami, level + 1);
  append_flavor_stat_line(tooltip, "[COLOR:Warning]", "Richness",
                          dishInfo.flavor.richness, showNext,
                          nextInfo.flavor.richness, level + 1);
  append_flavor_stat_line(tooltip, "[COLOR:Info]", "Freshness",
                          dishInfo.flavor.freshness, showNext,
                          nextInfo.flavor.freshness, level + 1);
  append_combat_stats_line(tooltip, dishInfo.flavor, level);

  if (!dishInfo.effects.empty()) {
    tooltip << "\n[COLOR:Gold]Effects:\n";
    tooltip << format_effects_section(dishInfo.effects);
  }

  if (showNext) {
    std::string nextPreview =
        format_next_level_effects(dishInfo.effects, nextInfo.effects,
                                  level + 1);
    if (!nextPreview.empty()) {
      tooltip << "\n[COLOR:Gold]Next Level:\n";
      tooltip << nextPreview;
    }
    std::string laterUnlocks = format_later_unlocks(dishType, level);
    if (!laterUnlocks.empty()) {
      tooltip << "\n[COLOR:Gold]Later Unlocks:\n";
      tooltip << laterUnlocks;
    }
  }

  return tooltip.str();
}
