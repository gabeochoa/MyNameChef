#pragma once

#include "brand_tag.h"
#include "course_tag.h"
#include "cuisine_tag.h"
#include "dish_archetype_tag.h"
#include <afterhours/ah.h>
#include <magic_enum/magic_enum.hpp>
#include <map>
#include <vector>

struct SynergyCounts : afterhours::BaseComponent {
  std::map<CourseTagType, int> course_counts;
  std::map<CuisineTagType, int> cuisine_counts;
  std::map<BrandTagType, int> brand_counts;
  std::map<DishArchetypeTagType, int> archetype_counts;

  // Manual thresholds per tag type
  std::map<CuisineTagType, std::vector<int>> cuisine_thresholds;
  std::map<BrandTagType, std::vector<int>> brand_thresholds;
  std::map<CourseTagType, std::vector<int>> course_thresholds;
  std::map<DishArchetypeTagType, std::vector<int>> archetype_thresholds;

  SynergyCounts() {
    // Initialize thresholds - can be customized per tag
    // Default: 2/4/6 for most tags
    for (auto cuisine : magic_enum::enum_values<CuisineTagType>()) {
      cuisine_thresholds[cuisine] = {2, 4, 6};
    }
    for (auto brand : magic_enum::enum_values<BrandTagType>()) {
      brand_thresholds[brand] = {2, 4, 6};
    }
    for (auto course : magic_enum::enum_values<CourseTagType>()) {
      course_thresholds[course] = {2, 4, 6};
    }
    for (auto archetype : magic_enum::enum_values<DishArchetypeTagType>()) {
      archetype_thresholds[archetype] = {2, 4, 6};
    }
  }

  void reset() {
    course_counts.clear();
    cuisine_counts.clear();
    brand_counts.clear();
    archetype_counts.clear();
  }

  // Get current threshold based on count
  int get_threshold(CuisineTagType cuisine) const {
    int count = get_count(cuisine);
    auto it = cuisine_thresholds.find(cuisine);
    if (it == cuisine_thresholds.end())
      return 0;
    for (int threshold : it->second) {
      if (count >= threshold)
        return threshold;
    }
    return 0;
  }

  // Get all thresholds for a tag
  std::vector<int> get_all_thresholds(CuisineTagType cuisine) const {
    auto it = cuisine_thresholds.find(cuisine);
    if (it == cuisine_thresholds.end())
      return {};
    return it->second;
  }

  std::vector<int> get_all_thresholds(BrandTagType brand) const {
    auto it = brand_thresholds.find(brand);
    if (it == brand_thresholds.end())
      return {};
    return it->second;
  }

  std::vector<int> get_all_thresholds(CourseTagType course) const {
    auto it = course_thresholds.find(course);
    if (it == course_thresholds.end())
      return {};
    return it->second;
  }

  std::vector<int> get_all_thresholds(DishArchetypeTagType archetype) const {
    auto it = archetype_thresholds.find(archetype);
    if (it == archetype_thresholds.end())
      return {};
    return it->second;
  }

  int get_count(CuisineTagType cuisine) const {
    auto it = cuisine_counts.find(cuisine);
    return it != cuisine_counts.end() ? it->second : 0;
  }

  int get_count(CourseTagType course) const {
    auto it = course_counts.find(course);
    return it != course_counts.end() ? it->second : 0;
  }

  int get_count(BrandTagType brand) const {
    auto it = brand_counts.find(brand);
    return it != brand_counts.end() ? it->second : 0;
  }

  int get_count(DishArchetypeTagType archetype) const {
    auto it = archetype_counts.find(archetype);
    return it != archetype_counts.end() ? it->second : 0;
  }
};
