#pragma once

#include "../components/applied_set_bonuses.h"
#include "../components/battle_synergy_counts.h"
#include "../components/cuisine_tag.h"
#include "../components/set_bonus_definitions.h"
#include "../font_info.h"
#include "../game_state_manager.h"
#include "../render_backend.h"
#include "../rl.h"
#include "../shop.h"
#include "../ui/text_formatting.h"
#include <afterhours/ah.h>
#include <magic_enum/magic_enum.hpp>

struct RenderBattleSynergyLegend : afterhours::System<> {
  virtual bool should_run(float) override {
    auto &gsm = GameStateManager::get();
    return gsm.active_screen == GameStateManager::Screen::Battle;
  }

  virtual void once(float) const override {
    auto battle_synergy_entity =
        afterhours::EntityHelper::get_singleton<BattleSynergyCounts>();
    if (!battle_synergy_entity.get().has<BattleSynergyCounts>()) {
      return;
    }

    const auto &battle_synergy =
        battle_synergy_entity.get().get<BattleSynergyCounts>();
    const auto &bonus_defs = synergy_bonuses::get_cuisine_bonus_definitions();

    float start_y = 170.0f;
    float line_height = 25.0f;
    float x = 20.0f;
    float text_size = font_sizes::Normal;
    int line_count = 0;

    for (const auto &[cuisine, count] : battle_synergy.player_cuisine_counts) {
      if (count < 2) {
        continue;
      }

      auto cuisine_it = bonus_defs.find(cuisine);
      if (cuisine_it == bonus_defs.end()) {
        continue;
      }

      int highest_threshold_reached = 0;
      int next_threshold = 6;
      for (const auto &[threshold, bonus] : cuisine_it->second) {
        if (count >= threshold && threshold > highest_threshold_reached) {
          highest_threshold_reached = threshold;
        }
        if (count < threshold && threshold < next_threshold) {
          next_threshold = threshold;
        }
      }

      if (highest_threshold_reached == 0) {
        continue;
      }

      std::string cuisine_name = std::string(magic_enum::enum_name(cuisine));
      std::string label = cuisine_name + ": " + std::to_string(count) + "/" +
                          std::to_string(next_threshold);

      float y = start_y + (line_count * line_height);
      raylib::Color text_color = text_formatting::TextFormatting::get_color(
          text_formatting::SemanticColor::Text,
          text_formatting::FormattingContext::Combat);
      render_backend::DrawTextWithActiveFont(label.c_str(), static_cast<int>(x),
                                             static_cast<int>(y), text_size,
                                             text_color);

      line_count++;
    }
  }
};
