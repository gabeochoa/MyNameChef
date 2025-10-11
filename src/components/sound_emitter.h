#pragma once

#include <afterhours/ah.h>
#include <unordered_map>

struct SoundEmitter : afterhours::BaseComponent {
  std::unordered_map<std::string, std::vector<std::string>> alias_names_by_base;
  std::unordered_map<std::string, size_t> next_alias_index_by_base;
  int default_alias_copies = 3;

  SoundEmitter() = default;
};