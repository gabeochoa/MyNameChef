#pragma once

#include "../components/brand_tag.h"
#include "../components/course_tag.h"
#include "../components/cuisine_tag.h"
#include "../components/dish_archetype_tag.h"
#include "../components/dish_level.h"
#include "../components/has_tooltip.h"
#include "../components/is_dish.h"
#include "../components/render_order.h"
#include "../components/synergy_counts.h"
#include "../components/transform.h"
#include "../font_info.h"
#include "../game_state_manager.h"
#include "../render_backend.h"
#include "../rl.h"
#include "../tooltip.h"
#include "../ui/text_formatting.h"
#include <afterhours/ah.h>
#include <magic_enum/magic_enum.hpp>
#include <map>
#include <sstream>

struct RenderTooltipSystem : afterhours::System<HasRenderOrder, HasTooltip> {
  GameStateManager::Screen current_screen;

public:
  virtual bool should_run(float) override {
    auto &gsm = GameStateManager::get();
    current_screen = gsm.active_screen;
    return GameStateManager::should_render_tooltips(gsm.active_screen);
  }

  virtual void for_each_with(const afterhours::Entity &entity,
                             const HasRenderOrder & /*render_order*/,
                             const HasTooltip &tooltip, float) const override {

    // Screen filtering is handled by should_run(); allow all render orders here

    const auto &transform = entity.get<Transform>();

    vec2 mouse_pos = afterhours::input::get_mouse_position();
    Rectangle entity_rect = transform.rect();

    bool mouse_over = CheckCollisionPointRec(
        raylib::Vector2{mouse_pos.x, mouse_pos.y},
        Rectangle{entity_rect.x, entity_rect.y, entity_rect.width,
                  entity_rect.height});

    if (!mouse_over)
      return;

    // Generate dynamic tooltip for dishes with level information
    std::string tooltip_text = tooltip.text;
    if (entity.has<IsDish>() && entity.has<DishLevel>()) {
      const auto &dish = entity.get<IsDish>();
      const auto &level = entity.get<DishLevel>();
      tooltip_text = generate_dish_tooltip_with_level(
          dish.type, level.level, level.merge_progress, level.merges_needed);

      // Add tags and synergy information
      std::ostringstream tag_info;
      bool has_tags = false;

      if (entity.has<CourseTag>()) {
        const auto &tag = entity.get<CourseTag>();
        bool first = true;
        for (auto course : magic_enum::enum_values<CourseTagType>()) {
          if (tag.has(course)) {
            if (first) {
              tag_info << "\n[COLOR:Gold]Course: [COLOR:Text]";
              first = false;
            } else {
              tag_info << ", ";
            }
            tag_info << magic_enum::enum_name(course);
          }
        }
        if (!first) {
          tag_info << "\n";
          has_tags = true;
        }
      }

      if (entity.has<CuisineTag>()) {
        const auto &tag = entity.get<CuisineTag>();
        bool first = true;
        CuisineTagType first_cuisine = CuisineTagType::Thai;
        for (auto cuisine : magic_enum::enum_values<CuisineTagType>()) {
          if (tag.has(cuisine)) {
            if (first) {
              tag_info << "[COLOR:Gold]Cuisine: [COLOR:Text]";
              first_cuisine = cuisine;
              first = false;
            } else {
              tag_info << ", ";
            }
            tag_info << magic_enum::enum_name(cuisine);
          }
        }
        if (!first) {
          tag_info << "\n";
          has_tags = true;

          // Add synergy count for first cuisine tag with all thresholds
          auto synergy_entity =
              afterhours::EntityHelper::get_singleton<SynergyCounts>();
          if (synergy_entity.get().has<SynergyCounts>()) {
            const auto &synergy = synergy_entity.get().get<SynergyCounts>();
            int count = synergy.get_count(first_cuisine);
            std::vector<int> thresholds =
                synergy.get_all_thresholds(first_cuisine);
            tag_info << "[COLOR:Info]Synergy: [COLOR:Text]" << count
                     << " dishes (";
            bool first_threshold = true;
            for (int threshold : thresholds) {
              if (!first_threshold) {
                tag_info << ", ";
              }
              if (count >= threshold) {
                // Current or achieved threshold - normal color
                tag_info << threshold;
              } else {
                // Future threshold - grayed out
                tag_info << "[COLOR:TextMuted]" << threshold << "[COLOR:Text]";
              }
              first_threshold = false;
            }
            tag_info << ")\n";
          }
        }
      }

      if (entity.has<BrandTag>()) {
        const auto &tag = entity.get<BrandTag>();
        bool first = true;
        BrandTagType first_brand = BrandTagType::LocalFarm;
        for (auto brand : magic_enum::enum_values<BrandTagType>()) {
          if (tag.has(brand)) {
            if (first) {
              tag_info << "[COLOR:Gold]Brand: [COLOR:Text]";
              first_brand = brand;
              first = false;
            } else {
              tag_info << ", ";
            }
            tag_info << magic_enum::enum_name(brand);
          }
        }
        if (!first) {
          tag_info << "\n";
          has_tags = true;

          // Add synergy count for first brand tag
          auto synergy_entity =
              afterhours::EntityHelper::get_singleton<SynergyCounts>();
          if (synergy_entity.get().has<SynergyCounts>()) {
            const auto &synergy = synergy_entity.get().get<SynergyCounts>();
            int count = synergy.get_count(first_brand);
            std::vector<int> thresholds =
                synergy.get_all_thresholds(first_brand);
            tag_info << "[COLOR:Info]Synergy: [COLOR:Text]" << count
                     << " dishes (";
            bool first_threshold = true;
            for (int threshold : thresholds) {
              if (!first_threshold) {
                tag_info << ", ";
              }
              if (count >= threshold) {
                tag_info << threshold;
              } else {
                tag_info << "[COLOR:TextMuted]" << threshold << "[COLOR:Text]";
              }
              first_threshold = false;
            }
            tag_info << ")\n";
          }
        }
      }

      if (entity.has<DishArchetypeTag>()) {
        const auto &tag = entity.get<DishArchetypeTag>();
        bool first = true;
        DishArchetypeTagType first_archetype = DishArchetypeTagType::Bread;
        for (auto archetype : magic_enum::enum_values<DishArchetypeTagType>()) {
          if (tag.has(archetype)) {
            if (first) {
              tag_info << "[COLOR:Gold]Archetype: [COLOR:Text]";
              first_archetype = archetype;
              first = false;
            } else {
              tag_info << ", ";
            }
            tag_info << magic_enum::enum_name(archetype);
          }
        }
        if (!first) {
          tag_info << "\n";
          has_tags = true;

          // Add synergy count for first archetype tag
          auto synergy_entity =
              afterhours::EntityHelper::get_singleton<SynergyCounts>();
          if (synergy_entity.get().has<SynergyCounts>()) {
            const auto &synergy = synergy_entity.get().get<SynergyCounts>();
            int count = synergy.get_count(first_archetype);
            std::vector<int> thresholds =
                synergy.get_all_thresholds(first_archetype);
            tag_info << "[COLOR:Info]Synergy: [COLOR:Text]" << count
                     << " dishes (";
            bool first_threshold = true;
            for (int threshold : thresholds) {
              if (!first_threshold) {
                tag_info << ", ";
              }
              if (count >= threshold) {
                tag_info << threshold;
              } else {
                tag_info << "[COLOR:TextMuted]" << threshold << "[COLOR:Text]";
              }
              first_threshold = false;
            }
            tag_info << ")\n";
          }
        }
      }

      if (has_tags) {
        tooltip_text += tag_info.str();
      }
    }

    float tooltip_x = mouse_pos.x + 10; // Offset from mouse
    float tooltip_y = mouse_pos.y - 30; // Above mouse

    // Calculate tooltip dimensions by measuring each line separately
    std::istringstream measure_stream(tooltip_text);
    std::string measure_line;
    float max_line_width = 0.0f;
    int line_count = 0;

    while (std::getline(measure_stream, measure_line)) {
      line_count++;
      // Remove color markers for width calculation using formatting system
      std::string clean_line =
          text_formatting::TextFormatting::strip_formatting_codes(measure_line);
      float line_width = render_backend::MeasureTextWithActiveFont(
          clean_line.c_str(), tooltip.font_size);
      if (line_width > max_line_width) {
        max_line_width = line_width;
      }
    }

    float tooltip_width = max_line_width + tooltip.padding * 2;
    float tooltip_height =
        (tooltip.font_size * line_count) + tooltip.padding * 2;

    // Get screen dimensions
    float screen_width = static_cast<float>(raylib::GetScreenWidth());
    float screen_height = static_cast<float>(raylib::GetScreenHeight());
    const float edge_margin = 10.0f;

    // Reposition tooltip to fit within screen bounds
    // Priority: Keep tooltip near cursor when possible

    // Check right edge: shift left if tooltip extends past right
    if (tooltip_x + tooltip_width > screen_width - edge_margin) {
      tooltip_x = screen_width - tooltip_width - edge_margin;
    }

    // Check left edge: shift right if tooltip extends past left
    if (tooltip_x < edge_margin) {
      tooltip_x = edge_margin;
    }

    // Check bottom edge: move above cursor if tooltip extends past bottom
    if (tooltip_y + tooltip_height > screen_height - edge_margin) {
      tooltip_y = mouse_pos.y - tooltip_height - 20;
    }

    // Check top edge: move below cursor if tooltip extends past top
    if (tooltip_y < edge_margin) {
      tooltip_y = mouse_pos.y + 20;
    }

    // Final validation: ensure tooltip is fully on screen
    // If tooltip is still too large for screen, clamp to edges
    if (tooltip_width > screen_width - edge_margin * 2) {
      tooltip_x = edge_margin;
    }
    if (tooltip_height > screen_height - edge_margin * 2) {
      tooltip_y = edge_margin;
    }

    // Draw tooltip background
    raylib::DrawRectangle(
        static_cast<int>(tooltip_x), static_cast<int>(tooltip_y),
        static_cast<int>(tooltip_width), static_cast<int>(tooltip_height),
        tooltip.background_color);

    // Draw tooltip border
    raylib::DrawRectangleLines(static_cast<int>(tooltip_x),
                               static_cast<int>(tooltip_y),
                               static_cast<int>(tooltip_width),
                               static_cast<int>(tooltip_height), raylib::WHITE);

    // Draw tooltip text with color support using formatting system
    text_formatting::render_formatted_text_multiline(
        tooltip_text, static_cast<int>(tooltip_x + tooltip.padding),
        static_cast<int>(tooltip_y + tooltip.padding), tooltip.font_size,
        text_formatting::FormattingContext::Tooltip, tooltip.text_color);
  }
};
