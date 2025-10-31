#include "translation_manager.h"
#include "log.h"
#include "magic_enum/magic_enum.hpp"
#include <map>

namespace translation_manager {

// Runtime translation maps
static std::map<strings::i18n, TranslatableString> english_translations = {
    {strings::i18n::play, translation_manager::TranslatableString(
                              "play", "Main menu button to start a new game")},
    {strings::i18n::about,
     translation_manager::TranslatableString(
         "about", "Main menu button to show game information")},
    {strings::i18n::exit, translation_manager::TranslatableString(
                              "exit", "Main menu button to quit the game")},
    {strings::i18n::start, translation_manager::TranslatableString(
                               "start", "Button to begin gameplay")},
    {strings::i18n::back,
     translation_manager::TranslatableString(
         "back", "Navigation button to return to previous screen")},
    {strings::i18n::continue_game,
     translation_manager::TranslatableString(
         "continue", "Button to continue after round ends")},
    {strings::i18n::quit, translation_manager::TranslatableString(
                              "quit", "Button to exit current game session")},
    {strings::i18n::settings,
     translation_manager::TranslatableString(
         "settings", "Main menu button to access game settings")},
    {strings::i18n::volume, translation_manager::TranslatableString(
                                "volume", "Generic volume setting label")},
    {strings::i18n::fullscreen,
     translation_manager::TranslatableString(
         "fullscreen", "Checkbox to toggle fullscreen mode")},
    {strings::i18n::resolution,
     translation_manager::TranslatableString(
         "resolution", "Dropdown to select screen resolution")},
    {strings::i18n::language,
     translation_manager::TranslatableString(
         "language", "Dropdown to select game language")},

    // Additional UI strings
    {strings::i18n::resume, translation_manager::TranslatableString(
                                "resume", "Button to unpause the game")},
    {strings::i18n::back_to_setup,
     translation_manager::TranslatableString(
         "back to setup", "Button to return to game setup from pause menu")},
    {strings::i18n::exit_game,
     translation_manager::TranslatableString(
         "exit game", "Button to quit current game from pause menu")},
    {strings::i18n::select_map,
     translation_manager::TranslatableString(
         "select map", "Button to choose a map for the game")},
    {strings::i18n::master_volume,
     translation_manager::TranslatableString("master volume",
                                             "Slider for overall game volume")},
    {strings::i18n::music_volume,
     translation_manager::TranslatableString(
         "music volume", "Slider for background music volume")},
    {strings::i18n::sfx_volume,
     translation_manager::TranslatableString(
         "sfx volume", "Slider for sound effects volume")},
    {strings::i18n::post_processing,
     translation_manager::TranslatableString(
         "post processing",
         "Checkbox to enable visual post-processing effects")},
    {strings::i18n::round_end,
     translation_manager::TranslatableString(
         "round end", "Title shown when a round finishes")},
    {strings::i18n::paused,
     translation_manager::TranslatableString(
         "paused", "Large text shown when game is paused")},
    {strings::i18n::unknown,
     translation_manager::TranslatableString(
         "unknown", "Fallback text for unknown game states")},
};

// Get translations for a specific language
const std::map<strings::i18n, TranslatableString> &
TranslationManager::get_translations_for_language(Language language) const {
  switch (language) {
  case Language::English:
    return english_translations;
  case Language::Korean:
    return english_translations;
  case Language::Japanese:
    return english_translations;
  }
}

TranslationManager::TranslationManager() {
  set_language(Language::English); // Default to English
}

std::map<strings::i18n, TranslatableString>::const_iterator
TranslationManager::find_translation(strings::i18n key) const {
  const auto &translations = get_translations_for_language(current_language);
  auto it = translations.find(key);
  if (it == translations.end()) {
    log_warn("Translation not found for key: {}", magic_enum::enum_name(key));
  }
  return it;
}

std::string TranslationManager::get_string(strings::i18n key) const {
  auto it = find_translation(key);
  if (it != get_translations_for_language(current_language).end()) {
    return it->second.get_text();
  }
  return "MISSING_TRANSLATION";
}

TranslatableString
TranslationManager::get_translatable_string(strings::i18n key) const {
  auto it = find_translation(key);
  if (it != get_translations_for_language(current_language).end()) {
    return translation_manager::TranslatableString(
        it->second.get_text(), it->second.get_description());
  }
  return translation_manager::TranslatableString("MISSING_TRANSLATION", true);
}

// TranslatableString constructor implementation
translation_manager::TranslatableString::TranslatableString(
    const strings::i18n &key) {
  auto &manager = TranslationManager::get();
  auto it = manager.find_translation(key);
  if (it !=
      manager.get_translations_for_language(manager.get_language()).end()) {
    content = it->second.get_text();
    description = it->second.description;
  } else {
    content = "MISSING_TRANSLATION";
    description = "Translation not found";
  }
}

void TranslationManager::set_language(Language language) {
  current_language = language;
  log_info("Language set to: {}", get_language_name());
}

// Load CJK fonts for all the strings this manager needs
void TranslationManager::load_cjk_fonts(
    afterhours::ui::FontManager & /* font_manager */,
    const std::string & /* font_file */) const {

  // Collect all unique codepoints from all CJK languages
  std::set<int> all_codepoints;

  // Add Latin alphabet (uppercase and lowercase) and numbers
  for (char c = 'A'; c <= 'Z'; ++c) {
    all_codepoints.insert(static_cast<int>(c));
  }
  for (char c = 'a'; c <= 'z'; ++c) {
    all_codepoints.insert(static_cast<int>(c));
  }
  for (char c = '0'; c <= '9'; ++c) {
    all_codepoints.insert(static_cast<int>(c));
  }

  // Add common punctuation characters
  const char punctuation[] = ".,!?;:()[]{}\"'`~@#$%^&*+-=_|\\/<>";
  for (char c : punctuation) {
    all_codepoints.insert(static_cast<int>(c));
  }

  for (const auto &lang : {Language::Korean, Language::Japanese}) {
    // Get all translations for this language
    const auto &translations = get_translations_for_language(lang);

    // Collect all unique characters from all translations
    std::string all_chars;
    for (auto it = translations.begin(); it != translations.end(); ++it) {
      all_chars += it->second.get_text();
    }

    // Extract codepoints
    size_t pos = 0;
    while (pos < all_chars.length()) {
      const unsigned char *bytes =
          reinterpret_cast<const unsigned char *>(all_chars.c_str() + pos);
      int codepoint = 0;
      int bytes_consumed = 0;

      // Simple UTF-8 decoding for the characters we need
      if (bytes[0] < 0x80) {
        codepoint = bytes[0];
        bytes_consumed = 1;
      } else if ((bytes[0] & 0xE0) == 0xC0 && pos + 1 < all_chars.length()) {
        codepoint = ((bytes[0] & 0x1F) << 6) | (bytes[1] & 0x3F);
        bytes_consumed = 2;
      } else if ((bytes[0] & 0xF0) == 0xE0 && pos + 2 < all_chars.length()) {
        codepoint = ((bytes[0] & 0x0F) << 12) | ((bytes[1] & 0x3F) << 6) |
                    (bytes[2] & 0x3F);
        bytes_consumed = 3;
      }

      if (bytes_consumed > 0 && codepoint > 0) {
        all_codepoints.insert(codepoint);
        pos += bytes_consumed;
      } else {
        pos++; // Skip invalid sequences
      }
    }
  }

  // Load all codepoints into both Korean and Japanese fonts
  if (!all_codepoints.empty()) {
    // Convert set to vector
    std::vector<int> codepoints(all_codepoints.begin(), all_codepoints.end());

    // Simplified - no font loading system
    // TODO: Implement font loading when Files system is available

    log_info("Loaded basic fonts with {} total codepoints for CJK support",
             codepoints.size());
  }
}

} // namespace translation_manager
