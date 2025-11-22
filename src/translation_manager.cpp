#include "translation_manager.h"
#include "font_info.h"
#include "log.h"
#include <afterhours/ah.h>
#include <map>
#include <vector>

namespace translation_manager {

using TranslationMap = std::map<strings::i18n, TranslatableString>;
using BaseTranslationMap = std::map<strings::i18n, BaseTranslatableString>;
using LanguageMap = TranslationPlugin::LanguageMap;

static LanguageMap build_translation_maps() {
  LanguageMap maps;

  TranslationMap english_translations = {
      {strings::i18n::play,
       TranslatableString("play", "Main menu button to start a new game")},
      {strings::i18n::about,
       TranslatableString("about",
                          "Main menu button to show game information")},
      {strings::i18n::exit,
       TranslatableString("exit", "Main menu button to quit the game")},
      {strings::i18n::start,
       TranslatableString("start", "Button to begin gameplay")},
      {strings::i18n::back,
       TranslatableString("back",
                          "Navigation button to return to previous screen")},
      {strings::i18n::continue_game,
       TranslatableString("continue", "Button to continue after round ends")},
      {strings::i18n::quit,
       TranslatableString("quit", "Button to exit current game session")},
      {strings::i18n::settings,
       TranslatableString("settings",
                          "Main menu button to access game settings")},
      {strings::i18n::volume,
       TranslatableString("volume", "Generic volume setting label")},
      {strings::i18n::fullscreen,
       TranslatableString("fullscreen", "Checkbox to toggle fullscreen mode")},
      {strings::i18n::resolution,
       TranslatableString("resolution",
                          "Dropdown to select screen resolution")},
      {strings::i18n::language,
       TranslatableString("language", "Dropdown to select game language")},

      // Additional UI strings
      {strings::i18n::resume,
       TranslatableString("resume", "Button to unpause the game")},
      {strings::i18n::back_to_setup,
       TranslatableString("back to setup",
                          "Button to return to game setup from pause menu")},
      {strings::i18n::exit_game,
       TranslatableString("exit game",
                          "Button to quit current game from pause menu")},
      {strings::i18n::select_map,
       TranslatableString("select map", "Button to choose a map for the game")},
      {strings::i18n::master_volume,
       TranslatableString("master volume", "Slider for overall game volume")},
      {strings::i18n::music_volume,
       TranslatableString("music volume",
                          "Slider for background music volume")},
      {strings::i18n::sfx_volume,
       TranslatableString("sfx volume", "Slider for sound effects volume")},
      {strings::i18n::post_processing,
       TranslatableString("post processing",
                          "Checkbox to enable visual post-processing effects")},
      {strings::i18n::round_end,
       TranslatableString("round end", "Title shown when a round finishes")},
      {strings::i18n::paused,
       TranslatableString("paused", "Large text shown when game is paused")},
      {strings::i18n::unknown,
       TranslatableString("unknown", "Fallback text for unknown game states")},
  };

  TranslationMap korean_translations = {
      {strings::i18n::play,
       TranslatableString("플레이", "새 게임을 시작하는 메인 메뉴 버튼")},
      {strings::i18n::about,
       TranslatableString("정보", "게임 정보를 표시하는 메인 메뉴 버튼")},
      {strings::i18n::exit,
       TranslatableString("종료", "게임을 종료하는 메인 메뉴 버튼")},
      {strings::i18n::start,
       TranslatableString("시작", "게임플레이를 시작하는 버튼")},
      {strings::i18n::back,
       TranslatableString("뒤로", "이전 화면으로 돌아가는 네비게이션 버튼")},
      {strings::i18n::continue_game,
       TranslatableString("계속", "라운드 종료 후 계속하는 버튼")},
      {strings::i18n::quit,
       TranslatableString("종료", "현재 게임 세션을 종료하는 버튼")},
      {strings::i18n::settings,
       TranslatableString("설정", "게임 설정에 접근하는 메인 메뉴 버튼")},
      {strings::i18n::language,
       TranslatableString("언어", "게임 언어를 선택하는 드롭다운")},
  };

  TranslationMap japanese_translations = {
      {strings::i18n::play,
       TranslatableString("プレイ",
                          "新しいゲームを開始するメインメニューボタン")},
      {strings::i18n::about,
       TranslatableString("情報", "ゲーム情報を表示するメインメニューボタン")},
      {strings::i18n::exit,
       TranslatableString("終了", "ゲームを終了するメインメニューボタン")},
      {strings::i18n::start,
       TranslatableString("開始", "ゲームプレイを開始するボタン")},
      {strings::i18n::back,
       TranslatableString("戻る", "前の画面に戻るナビゲーションボタン")},
      {strings::i18n::continue_game,
       TranslatableString("続ける", "ラウンド終了後に続けるボタン")},
      {strings::i18n::quit,
       TranslatableString("終了", "現在のゲームセッションを終了するボタン")},
      {strings::i18n::settings,
       TranslatableString("設定",
                          "ゲーム設定にアクセスするメインメニューボタン")},
      {strings::i18n::language,
       TranslatableString("言語", "ゲーム言語を選択するドロップダウン")},
  };

  // Convert TranslatableString maps to BaseTranslatableString maps
  BaseTranslationMap english_base, korean_base, japanese_base;
  for (const auto &[key, value] : english_translations) {
    english_base[key] =
        BaseTranslatableString(value.get_text(), value.get_description());
  }
  for (const auto &[key, value] : korean_translations) {
    korean_base[key] =
        BaseTranslatableString(value.get_text(), value.get_description());
  }
  for (const auto &[key, value] : japanese_translations) {
    japanese_base[key] =
        BaseTranslatableString(value.get_text(), value.get_description());
  }

  maps[Language::English] = std::move(english_base);
  maps[Language::Korean] = std::move(korean_base);
  maps[Language::Japanese] = std::move(japanese_base);

  return maps;
}

static LanguageMap g_translation_maps = build_translation_maps();

void initialize_translation_plugin(afterhours::Entity &entity) {
  TranslationPlugin::add_singleton_components(
      entity, g_translation_maps, Language::English, translation_param);
}

TranslatableString::TranslatableString(const strings::i18n &key) {
  auto base = TranslationPlugin::get_translatable_string(key);
  *static_cast<BaseTranslatableString *>(this) = base;
}

} // namespace translation_manager
