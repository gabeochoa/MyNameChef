#pragma once

#include <string>

enum FontID {
  English,
  Korean,
  Japanese,
  raylibFont,
  SYMBOL_FONT,
};

std::string get_font_name(FontID id);

namespace translation_manager {
FontID get_font_for_language();
}

inline FontID get_active_font_id() {
  return translation_manager::get_font_for_language();
}

inline std::string get_active_font_name() {
  return get_font_name(get_active_font_id());
}

namespace font_sizes {
// TODO: Add resolution-based scaling for font sizes
// For accessibility reasons, we want to make sure we are drawing text that's
// larger than 28px @ 1080p (which scales to ~18.67px @ 720p).
// Reference: pharmasea/src/engine/ui/ui_internal.h:58-69
// Current sizes are fixed for 720p baseline - need to scale up for higher
// resolutions
static constexpr float Small = 20.f;  // Minimum accessible size at 720p
static constexpr float Normal = 20.f; // Minimum accessible size at 720p
static constexpr float Medium = 24.f;
static constexpr float Large = 36.f;
static constexpr float Title = 48.f;
} // namespace font_sizes