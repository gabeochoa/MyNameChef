#pragma once

#include "external.h"  // Must be first to define AFTER_HOURS_USE_RAYLIB and type macros
#include "font_info.h"
#include "strings.h"
#include <afterhours/src/plugins/translation.h>
#include <afterhours/src/plugins/ui/components.h>
#include <fmt/format.h>
#include <functional>
#include <magic_enum/magic_enum.hpp>
#include <map>
#include <string>

namespace translation_manager {

using Language = afterhours::translation::Language;

enum struct i18nParam {
  number_count,
  number_time,
  number_ordinal,
  dish_name,
  synergy_name,
  stat_name,
  currency_amount,
  player_name,

  Count
};

// Parameter name mapping for fmt::format
const std::map<i18nParam, std::string> translation_param = {
    {i18nParam::number_count, "number_count"},
    {i18nParam::number_time, "number_time"},
    {i18nParam::number_ordinal, "number_ordinal"},
    {i18nParam::dish_name, "dish_name"},
    {i18nParam::synergy_name, "synergy_name"},
    {i18nParam::stat_name, "stat_name"},
    {i18nParam::currency_amount, "currency_amount"},
    {i18nParam::player_name, "player_name"},
};

using TranslationPlugin = afterhours::translation::translation_plugin<
    strings::i18n, i18nParam, FontID, decltype(&get_font_name)>;

using BaseTranslatableString =
    afterhours::translation::TranslatableString<i18nParam>;

struct TranslatableString : public BaseTranslatableString {
  using BaseTranslatableString::BaseTranslatableString;

  explicit TranslatableString(const strings::i18n &key);

  auto &set_param(const i18nParam &param, const std::string &arg) {
    return BaseTranslatableString::set_param(param, arg, translation_param);
  }

  template <typename T> auto &set_param(const i18nParam &param, const T &arg) {
    return BaseTranslatableString::set_param(param, arg, translation_param);
  }

  auto &set_param(const i18nParam &param, const BaseTranslatableString &arg) {
    return BaseTranslatableString::set_param(param, arg, translation_param);
  }

  fmt::dynamic_format_arg_store<fmt::format_context> get_params() const {
    return BaseTranslatableString::get_params(translation_param);
  }

  operator std::string() const {
    if (skip_translate())
      return underlying_TL_ONLY();
    if (is_formatted()) {
      return fmt::vformat(underlying_TL_ONLY(), get_params());
    }
    return underlying_TL_ONLY();
  }
};

// Helper function to create parameter for fmt::format
template <typename T>
fmt::detail::named_arg<char, T> create_param(const i18nParam &param,
                                             const T &arg) {
  if (!translation_param.contains(param)) {
    // Fallback to enum name if parameter missing
    return fmt::arg("unknown", arg);
  }
  const char *param_name = translation_param.at(param).c_str();
  return fmt::arg(param_name, arg);
}

class TranslationManager {
public:
  static TranslationManager &get() {
    static TranslationManager instance;
    return instance;
  }

  std::string get_string(strings::i18n key) const {
    return TranslationPlugin::get_string(key);
  }

  TranslatableString get_translatable_string(strings::i18n key) const {
    auto base = TranslationPlugin::get_translatable_string(key);
    return TranslatableString(base.get_text(), base.get_description());
  }

  std::map<strings::i18n, BaseTranslatableString>::const_iterator
  find_translation(strings::i18n key) const {
    auto *provides = afterhours::EntityHelper::get_singleton_cmp<
        TranslationPlugin::ProvidesTranslation>();
    if (!provides) {
      static std::map<strings::i18n, BaseTranslatableString> empty;
      return empty.end();
    }
    const auto &trans_map =
        provides->get_translations_for_language(provides->current_language);
    return trans_map.find(key);
  }

  const std::map<strings::i18n, BaseTranslatableString> &
  get_translations_for_language(Language language) const {
    auto *provides = afterhours::EntityHelper::get_singleton_cmp<
        TranslationPlugin::ProvidesTranslation>();
    if (!provides) {
      static std::map<strings::i18n, BaseTranslatableString> empty;
      return empty;
    }
    return provides->get_translations_for_language(language);
  }

  FontID get_font_for_language() const {
    return TranslationPlugin::get_font_for_language(
        get_language(), [](Language lang) -> FontID {
          switch (lang) {
          case Language::Korean:
            return FontID::Korean;
          case Language::Japanese:
            return FontID::Japanese;
          case Language::English:
          default:
            return FontID::English;
          }
        });
  }

  void set_language(Language language) {
    TranslationPlugin::set_language(language);
  }

  Language get_language() const { return TranslationPlugin::get_language(); }

  std::string get_language_name() const {
    return TranslationPlugin::get_language_name(get_language());
  }

  static std::string get_language_name(Language language) {
    return TranslationPlugin::get_language_name(language);
  }

  static std::vector<std::string> get_available_languages() {
    return TranslationPlugin::get_available_languages();
  }

  static size_t get_language_index(Language language) {
    return TranslationPlugin::get_language_index(language);
  }

  template <typename FontManager>
  void load_cjk_fonts(FontManager &font_manager,
                      const std::string &font_file) const {
    TranslationPlugin::load_cjk_fonts(font_manager, font_file, get_font_name,
                                      [](Language lang) -> FontID {
                                        switch (lang) {
                                        case Language::Korean:
                                          return FontID::Korean;
                                        case Language::Japanese:
                                          return FontID::Japanese;
                                        case Language::English:
                                        default:
                                          return FontID::English;
                                        }
                                      });
  }
};

// Global get_string function
inline std::string get_string(strings::i18n key) {
  return TranslationManager::get().get_string(key);
}

inline TranslatableString get_translatable_string(strings::i18n key) {
  return TranslatableString(key);
}

// Global get_font_for_language function
inline FontID get_font_for_language() {
  return TranslationManager::get().get_font_for_language();
}

// Global set_language function
inline void set_language(Language language) {
  TranslationManager::get().set_language(language);
}

// Global get_language function
inline Language get_language() {
  return TranslationManager::get().get_language();
}

// Global get_language_name function
inline std::string get_language_name(translation_manager::Language language) {
  return TranslationManager::get_language_name(language);
}

// Global get_available_languages function
inline std::vector<std::string> get_available_languages() {
  return TranslationManager::get_available_languages();
}

// Global get_language_index function
inline size_t get_language_index(translation_manager::Language language) {
  return TranslationManager::get_language_index(language);
}

[[nodiscard]] inline TranslatableString NO_TRANSLATE(const std::string &s) {
  return TranslatableString{s, true};
}

[[nodiscard]] inline std::string
translate_formatted(const TranslatableString &trs) {
  return static_cast<std::string>(trs);
}

void initialize_translation_plugin(afterhours::Entity &entity);

} // namespace translation_manager
