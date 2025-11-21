#pragma once

#include "../font_info.h"
#include "../render_backend.h"
#include "../rl.h"
#include <afterhours/src/plugins/color.h>
#include <afterhours/src/plugins/ui/theme_defaults.h>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace text_formatting {

enum struct FormattingContext { UI, Combat, Tooltip, HUD, Results };

enum struct SemanticColor {
  Primary,
  Secondary,
  Accent,
  Success,
  Warning,
  Error,
  Info,
  Text,
  TextMuted,
  Gold,
  Health,
  Positive,
  Negative
};

struct FormattingStyle {
  raylib::Color color;
  bool is_bold;
  bool is_italic;

  FormattingStyle() : color(raylib::WHITE), is_bold(false), is_italic(false) {}
  FormattingStyle(raylib::Color c, bool bold = false, bool italic = false)
      : color(c), is_bold(bold), is_italic(italic) {}
};

struct FormattedSegment {
  std::string text;
  raylib::Color color;
  bool is_bold;
  bool is_italic;

  FormattedSegment()
      : text(""), color(raylib::WHITE), is_bold(false), is_italic(false) {}
  FormattedSegment(const std::string &t, raylib::Color c, bool bold = false,
                   bool italic = false)
      : text(t), color(c), is_bold(bold), is_italic(italic) {}
};

class TextFormatting {
public:
  static raylib::Color get_color(SemanticColor semantic,
                                 FormattingContext context) {
    if (context == FormattingContext::UI) {
      return get_ui_color(semantic);
    } else if (context == FormattingContext::Combat) {
      return get_combat_color(semantic);
    } else if (context == FormattingContext::Tooltip) {
      return get_tooltip_color(semantic);
    } else if (context == FormattingContext::HUD) {
      return get_hud_color(semantic);
    } else if (context == FormattingContext::Results) {
      return get_results_color(semantic);
    }
    return raylib::WHITE;
  }

  static std::vector<FormattedSegment>
  parse_formatting_codes(const std::string &text, FormattingContext context,
                         raylib::Color default_color = raylib::WHITE) {
    std::vector<FormattedSegment> segments;
    std::string current_text;
    raylib::Color current_color = default_color;
    bool current_bold = false;
    bool current_italic = false;

    size_t pos = 0;
    while (pos < text.length()) {
      if (text[pos] == '[') {
        size_t end_pos = text.find(']', pos);
        if (end_pos != std::string::npos) {
          std::string marker = text.substr(pos, end_pos - pos + 1);

          if (marker == "[BOLD]") {
            if (!current_text.empty()) {
              segments.push_back(FormattedSegment(
                  current_text, current_color, current_bold, current_italic));
              current_text.clear();
            }
            current_bold = true;
            pos = end_pos + 1;
            continue;
          } else if (marker == "[/BOLD]") {
            if (!current_text.empty()) {
              segments.push_back(FormattedSegment(
                  current_text, current_color, current_bold, current_italic));
              current_text.clear();
            }
            current_bold = false;
            pos = end_pos + 1;
            continue;
          } else if (marker == "[ITALIC]") {
            if (!current_text.empty()) {
              segments.push_back(FormattedSegment(
                  current_text, current_color, current_bold, current_italic));
              current_text.clear();
            }
            current_italic = true;
            pos = end_pos + 1;
            continue;
          } else if (marker == "[/ITALIC]") {
            if (!current_text.empty()) {
              segments.push_back(FormattedSegment(
                  current_text, current_color, current_bold, current_italic));
              current_text.clear();
            }
            current_italic = false;
            pos = end_pos + 1;
            continue;
          } else if (marker == "[RESET]") {
            if (!current_text.empty()) {
              segments.push_back(FormattedSegment(
                  current_text, current_color, current_bold, current_italic));
              current_text.clear();
            }
            current_color = default_color;
            current_bold = false;
            current_italic = false;
            pos = end_pos + 1;
            continue;
          } else if (marker.find("[COLOR:") == 0 && marker.length() > 7) {
            std::string color_name = marker.substr(7, marker.length() - 8);
            SemanticColor semantic = parse_semantic_color(color_name);
            if (!current_text.empty()) {
              segments.push_back(FormattedSegment(
                  current_text, current_color, current_bold, current_italic));
              current_text.clear();
            }
            current_color = get_color(semantic, context);
            pos = end_pos + 1;
            continue;
          } else {
            // If marker is not recognized, skip it (don't add to text)
            pos = end_pos + 1;
            continue;
          }
        } else {
          // No matching ']' found, treat '[' as regular text
          current_text += text[pos];
          pos++;
          continue;
        }
      }

      current_text += text[pos];
      pos++;
    }

    if (!current_text.empty()) {
      segments.push_back(FormattedSegment(current_text, current_color,
                                          current_bold, current_italic));
    }

    return segments;
  }

  static std::string strip_formatting_codes(const std::string &text) {
    std::string result = text;
    static const std::vector<std::string> markers = {
        "[BOLD]",           "[/BOLD]",          "[ITALIC]",
        "[/ITALIC]",        "[RESET]",          "[COLOR:Gold]",
        "[COLOR:Error]",    "[COLOR:Success]",  "[COLOR:Info]",
        "[COLOR:Warning]",  "[COLOR:Text]",     "[COLOR:Health]",
        "[COLOR:Positive]", "[COLOR:Negative]", "[COLOR:TextMuted]",
        "[COLOR:Accent]",   "[COLOR:Primary]",  "[COLOR:Secondary]"};

    for (const auto &marker : markers) {
      size_t pos = 0;
      while ((pos = result.find(marker, pos)) != std::string::npos) {
        result.erase(pos, marker.length());
      }
    }

    size_t pos = 0;
    while ((pos = result.find("[COLOR:", pos)) != std::string::npos) {
      size_t end_pos = result.find("]", pos);
      if (end_pos != std::string::npos) {
        result.erase(pos, end_pos - pos + 1);
      } else {
        pos++;
      }
    }

    return result;
  }

private:
  static raylib::Color get_ui_color(SemanticColor semantic) {
    auto theme = afterhours::ui::imm::ThemeDefaults::get().get_theme();

    switch (semantic) {
    case SemanticColor::Primary:
      return theme.from_usage(afterhours::ui::Theme::Usage::Primary);
    case SemanticColor::Secondary:
      return theme.from_usage(afterhours::ui::Theme::Usage::Secondary);
    case SemanticColor::Accent:
      return theme.from_usage(afterhours::ui::Theme::Usage::Accent);
    case SemanticColor::Error:
      return theme.from_usage(afterhours::ui::Theme::Usage::Error);
    case SemanticColor::Text:
      return theme.from_usage(afterhours::ui::Theme::Usage::Font);
    case SemanticColor::TextMuted:
      return theme.from_usage(afterhours::ui::Theme::Usage::DarkFont);
    case SemanticColor::Success:
      return afterhours::colors::UI_GREEN;
    case SemanticColor::Warning:
      return raylib::YELLOW;
    case SemanticColor::Info:
      return afterhours::colors::UI_BLUE;
    case SemanticColor::Gold:
      return raylib::GOLD;
    case SemanticColor::Health:
      return afterhours::colors::UI_RED;
    case SemanticColor::Positive:
      return afterhours::colors::UI_GREEN;
    case SemanticColor::Negative:
      return afterhours::colors::UI_RED;
    }
    return raylib::WHITE;
  }

  static raylib::Color get_combat_color(SemanticColor semantic) {
    switch (semantic) {
    case SemanticColor::Error:
    case SemanticColor::Negative:
    case SemanticColor::Health:
      return raylib::RED;
    case SemanticColor::Success:
    case SemanticColor::Positive:
      return raylib::GREEN;
    case SemanticColor::Warning:
      return raylib::YELLOW;
    case SemanticColor::Info:
      return raylib::SKYBLUE;
    case SemanticColor::Text:
      return raylib::WHITE;
    case SemanticColor::TextMuted:
      return raylib::GRAY;
    case SemanticColor::Gold:
      return raylib::GOLD;
    case SemanticColor::Accent:
      return raylib::ORANGE;
    case SemanticColor::Primary:
    case SemanticColor::Secondary:
      return raylib::WHITE;
    default:
      return raylib::WHITE;
    }
  }

  static raylib::Color get_tooltip_color(SemanticColor semantic) {
    switch (semantic) {
    case SemanticColor::Gold:
      return raylib::GOLD;
    case SemanticColor::Error:
    case SemanticColor::Negative:
      return raylib::RED;
    case SemanticColor::Success:
    case SemanticColor::Positive:
      return raylib::GREEN;
    case SemanticColor::Info:
      return raylib::SKYBLUE;
    case SemanticColor::Warning:
      return raylib::YELLOW;
    case SemanticColor::Text:
      return raylib::WHITE;
    case SemanticColor::TextMuted:
      return raylib::GRAY;
    case SemanticColor::Health:
      return raylib::RED;
    case SemanticColor::Primary:
    case SemanticColor::Secondary:
    case SemanticColor::Accent:
      return raylib::WHITE;
    default:
      return raylib::WHITE;
    }
  }

  static raylib::Color get_hud_color(SemanticColor semantic) {
    switch (semantic) {
    case SemanticColor::Gold:
      return raylib::GOLD;
    case SemanticColor::Health:
    case SemanticColor::Error:
      return raylib::RED;
    case SemanticColor::Text:
      return raylib::WHITE;
    case SemanticColor::Success:
      return raylib::GREEN;
    case SemanticColor::Warning:
      return raylib::YELLOW;
    case SemanticColor::Info:
      return raylib::SKYBLUE;
    case SemanticColor::TextMuted:
      return raylib::GRAY;
    case SemanticColor::Primary:
    case SemanticColor::Secondary:
    case SemanticColor::Accent:
    case SemanticColor::Positive:
    case SemanticColor::Negative:
      return raylib::WHITE;
    default:
      return raylib::WHITE;
    }
  }

  static raylib::Color get_results_color(SemanticColor semantic) {
    switch (semantic) {
    case SemanticColor::Accent:
      return raylib::YELLOW;
    case SemanticColor::Success:
      return raylib::GREEN;
    case SemanticColor::Error:
      return raylib::RED;
    case SemanticColor::Warning:
      return raylib::YELLOW;
    case SemanticColor::Text:
      return raylib::WHITE;
    case SemanticColor::TextMuted:
      return raylib::GRAY;
    case SemanticColor::Primary:
    case SemanticColor::Secondary:
    case SemanticColor::Info:
    case SemanticColor::Gold:
    case SemanticColor::Health:
    case SemanticColor::Positive:
    case SemanticColor::Negative:
    default:
      return raylib::WHITE;
    }
  }

  static SemanticColor parse_semantic_color(const std::string &name) {
    if (name == "Gold")
      return SemanticColor::Gold;
    if (name == "Error")
      return SemanticColor::Error;
    if (name == "Success")
      return SemanticColor::Success;
    if (name == "Warning")
      return SemanticColor::Warning;
    if (name == "Info")
      return SemanticColor::Info;
    if (name == "Text")
      return SemanticColor::Text;
    if (name == "Health")
      return SemanticColor::Health;
    if (name == "Positive")
      return SemanticColor::Positive;
    if (name == "Negative")
      return SemanticColor::Negative;
    if (name == "Primary")
      return SemanticColor::Primary;
    if (name == "Secondary")
      return SemanticColor::Secondary;
    if (name == "Accent")
      return SemanticColor::Accent;
    return SemanticColor::Text;
  }
};

inline std::string format_stat_change(int value) {
  if (value > 0) {
    return "[COLOR:Positive]+" + std::to_string(value) + "[/COLOR]";
  } else if (value < 0) {
    return "[COLOR:Negative]" + std::to_string(value) + "[/COLOR]";
  }
  return std::to_string(value);
}

inline std::string
format_currency(int amount,
                FormattingContext = FormattingContext::HUD) {
  return "[COLOR:Gold]" + std::to_string(amount) + " gold[/COLOR]";
}

inline std::string
format_health(int current, int max,
              FormattingContext = FormattingContext::HUD) {
  float ratio =
      max > 0 ? static_cast<float>(current) / static_cast<float>(max) : 0.0f;
  std::string health_text = std::to_string(current) + "/" + std::to_string(max);

  if (ratio < 0.3f) {
    return "[COLOR:Error]" + health_text + "[/COLOR]";
  } else if (ratio < 0.6f) {
    return "[COLOR:Warning]" + health_text + "[/COLOR]";
  } else {
    return "[COLOR:Success]" + health_text + "[/COLOR]";
  }
}

inline void render_formatted_text(const std::string &text, int posX, int posY,
                                  float fontSize, FormattingContext context,
                                  raylib::Color default_color = raylib::WHITE) {
  std::vector<FormattedSegment> segments =
      TextFormatting::parse_formatting_codes(text, context, default_color);

  float current_x = static_cast<float>(posX);
  float current_y = static_cast<float>(posY);

  for (const auto &segment : segments) {
    if (!segment.text.empty()) {
      render_backend::DrawTextWithActiveFont(
          segment.text.c_str(), static_cast<int>(current_x),
          static_cast<int>(current_y), fontSize, segment.color);
      current_x += render_backend::MeasureTextWithActiveFont(
          segment.text.c_str(), fontSize);
    }
  }
}

inline float measure_formatted_text(const std::string &text, float fontSize) {
  std::string plain_text = TextFormatting::strip_formatting_codes(text);
  return render_backend::MeasureTextWithActiveFont(plain_text.c_str(),
                                                   fontSize);
}

inline void
render_formatted_text_multiline(const std::string &text, int posX, int posY,
                                float fontSize, FormattingContext context,
                                raylib::Color default_color = raylib::WHITE) {
  float current_y = static_cast<float>(posY);
  std::istringstream text_stream(text);
  std::string line;

  while (std::getline(text_stream, line)) {
    float current_x = static_cast<float>(posX);

    std::vector<FormattedSegment> segments =
        TextFormatting::parse_formatting_codes(line, context, default_color);

    for (const auto &segment : segments) {
      if (!segment.text.empty()) {
        render_backend::DrawTextWithActiveFont(
            segment.text.c_str(), static_cast<int>(current_x),
            static_cast<int>(current_y), fontSize, segment.color);
        current_x += render_backend::MeasureTextWithActiveFont(
            segment.text.c_str(), fontSize);
      }
    }

    current_y += fontSize;
  }
}

} // namespace text_formatting
