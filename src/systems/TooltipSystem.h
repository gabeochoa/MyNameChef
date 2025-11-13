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
#include "../game_state_manager.h"
#include "../rl.h"
#include "../tooltip.h"
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
              tag_info << "\n[GOLD]Course: [WHITE]";
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
              tag_info << "[GOLD]Cuisine: [WHITE]";
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
          auto synergy_entity = afterhours::EntityHelper::get_singleton<SynergyCounts>();
          if (synergy_entity.get().has<SynergyCounts>()) {
            const auto &synergy = synergy_entity.get().get<SynergyCounts>();
            int count = synergy.get_count(first_cuisine);
            std::vector<int> thresholds = synergy.get_all_thresholds(first_cuisine);
            tag_info << "[CYAN]Synergy: [WHITE]" << count << " dishes (";
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
                tag_info << "[GRAY]" << threshold << "[WHITE]";
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
              tag_info << "[GOLD]Brand: [WHITE]";
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
          auto synergy_entity = afterhours::EntityHelper::get_singleton<SynergyCounts>();
          if (synergy_entity.get().has<SynergyCounts>()) {
            const auto &synergy = synergy_entity.get().get<SynergyCounts>();
            int count = synergy.get_count(first_brand);
            std::vector<int> thresholds = synergy.get_all_thresholds(first_brand);
            tag_info << "[CYAN]Synergy: [WHITE]" << count << " dishes (";
            bool first_threshold = true;
            for (int threshold : thresholds) {
              if (!first_threshold) {
                tag_info << ", ";
              }
              if (count >= threshold) {
                tag_info << threshold;
              } else {
                tag_info << "[GRAY]" << threshold << "[WHITE]";
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
              tag_info << "[GOLD]Archetype: [WHITE]";
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
          auto synergy_entity = afterhours::EntityHelper::get_singleton<SynergyCounts>();
          if (synergy_entity.get().has<SynergyCounts>()) {
            const auto &synergy = synergy_entity.get().get<SynergyCounts>();
            int count = synergy.get_count(first_archetype);
            std::vector<int> thresholds = synergy.get_all_thresholds(first_archetype);
            tag_info << "[CYAN]Synergy: [WHITE]" << count << " dishes (";
            bool first_threshold = true;
            for (int threshold : thresholds) {
              if (!first_threshold) {
                tag_info << ", ";
              }
              if (count >= threshold) {
                tag_info << threshold;
              } else {
                tag_info << "[GRAY]" << threshold << "[WHITE]";
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

    // Measure text to calculate tooltip size
    float text_width = raylib::MeasureText(tooltip_text.c_str(),
                                           static_cast<int>(tooltip.font_size));
    float tooltip_width = text_width + tooltip.padding * 2;

    // Calculate height based on number of lines
    int line_count = 1; // Start with 1 for the first line
    for (char c : tooltip_text) {
      if (c == '\n') {
        line_count++;
      }
    }
    float tooltip_height =
        (tooltip.font_size * line_count) + tooltip.padding * 2;

    // Keep tooltip on screen
    float screen_width = raylib::GetScreenWidth();

    if (tooltip_x + tooltip_width > screen_width) {
      tooltip_x = screen_width - tooltip_width - 10;
    }
    if (tooltip_y < 0) {
      tooltip_y = mouse_pos.y + 20; // Below mouse if no space above
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

    // Draw tooltip text with color support
    float current_y = tooltip_y + tooltip.padding;
    std::istringstream text_stream(tooltip_text);
    std::string line;

    // Color mapping
    std::map<std::string, raylib::Color> color_map = {
        {"[GOLD]", raylib::GOLD},     {"[RED]", raylib::RED},
        {"[GREEN]", raylib::GREEN},   {"[BLUE]", raylib::BLUE},
        {"[ORANGE]", raylib::ORANGE}, {"[PURPLE]", raylib::PURPLE},
        {"[CYAN]", raylib::SKYBLUE},  {"[YELLOW]", raylib::YELLOW},
        {"[WHITE]", raylib::WHITE},   {"[GRAY]", raylib::GRAY}};

    while (std::getline(text_stream, line)) {
      float current_x = tooltip_x + tooltip.padding;
      raylib::Color current_color = tooltip.text_color; // Default color

      // Parse line for color markers and render segments
      size_t pos = 0;
      while (pos < line.length()) {
        // Find the next color marker
        size_t next_marker_pos = std::string::npos;
        std::string next_marker;
        raylib::Color next_color = current_color;

        for (const auto &[marker, color] : color_map) {
          size_t marker_pos = line.find(marker, pos);
          if (marker_pos != std::string::npos &&
              (next_marker_pos == std::string::npos ||
               marker_pos < next_marker_pos)) {
            next_marker_pos = marker_pos;
            next_marker = marker;
            next_color = color;
          }
        }

        // Extract text segment before next marker (or to end of line)
        size_t segment_end = (next_marker_pos != std::string::npos)
                                 ? next_marker_pos
                                 : line.length();
        std::string segment = line.substr(pos, segment_end - pos);

        // Draw the segment if it's not empty
        if (!segment.empty()) {
          raylib::DrawText(segment.c_str(), static_cast<int>(current_x),
                           static_cast<int>(current_y),
                           static_cast<int>(tooltip.font_size), current_color);
          current_x += raylib::MeasureText(segment.c_str(),
                                            static_cast<int>(tooltip.font_size));
        }

        // Update position and color for next segment
        if (next_marker_pos != std::string::npos) {
          pos = next_marker_pos + next_marker.length();
          current_color = next_color;
        } else {
          pos = line.length();
        }
      }

      current_y += tooltip.font_size;
    }
  }
};
