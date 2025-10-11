#pragma once

#include "../shader_types.h"
#include <afterhours/ah.h>

struct HasShader : afterhours::BaseComponent {
  std::vector<ShaderType> shaders;
  mutable std::optional<std::set<ShaderType>> shader_set_cache;

  HasShader() = default;
  explicit HasShader(ShaderType shader) { shaders.push_back(shader); }
  explicit HasShader(const std::vector<ShaderType> &shader_list)
      : shaders(shader_list) {}

  bool has_shader(ShaderType shader) const {
    if (!shader_set_cache.has_value()) {
      shader_set_cache = std::set<ShaderType>(shaders.begin(), shaders.end());
    }
    return shader_set_cache->contains(shader);
  }

  void add_shader(ShaderType shader) {
    shaders.push_back(shader);
    shader_set_cache.reset(); // Invalidate cache
  }

  void remove_shader(ShaderType shader) {
    shaders.erase(std::remove(shaders.begin(), shaders.end(), shader),
                  shaders.end());
    shader_set_cache.reset(); // Invalidate cache
  }

  void clear_shaders() {
    shaders.clear();
    shader_set_cache.reset(); // Invalidate cache
  }
};
