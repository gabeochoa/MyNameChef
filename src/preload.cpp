#include "preload.h"

#include <iostream>
#include <sstream>
#include <vector>

#include "log.h"
#include "rl.h"

#include "font_info.h"
#include "settings.h"

#include "music_library.h"
#include "shader_library.h"
#include "sound_library.h"
#include "texture_library.h"
#include "translation_manager.h"
#include "ui/ui_systems.h"

using namespace afterhours;
// for HasTexture
#include "components/has_camera.h"
#include "components/sound_emitter.h"
#include "input_mapping.h"

std::string get_font_name(FontID id) {
  switch (id) {
  case FontID::English:
    // return "eqprorounded-regular.ttf";
    return "NotoSansMonoCJKkr-Bold.otf";
  case FontID::Korean:
    return "NotoSansMonoCJKkr-Bold.otf";
  case FontID::Japanese:
    return "NotoSansMonoCJKjp-Bold.otf";
  case FontID::raylibFont:
    return afterhours::ui::UIComponent::DEFAULT_FONT;
  case FontID::SYMBOL_FONT:
    return "NotoSansMonoCJKkr-Bold.otf";
    // return "eqprorounded-regular.ttf";
  }
  return afterhours::ui::UIComponent::DEFAULT_FONT;
}

static void load_gamepad_mappings() {
  std::ifstream ifs(
      Files::get().fetch_resource_path("", "gamecontrollerdb.txt").c_str());
  if (!ifs.is_open()) {
    log_warn("failed to load game controller db");
    return;
  }
  std::stringstream buffer;
  buffer << ifs.rdbuf();
  input::set_gamepad_mappings(buffer.str().c_str());
}

Preload::Preload() {}

Preload &Preload::init(const char *title) { return init(title, false); }

Preload &Preload::init(const char *title, bool headless) {

  int width = Settings::get().get_screen_width();
  int height = Settings::get().get_screen_height();

  // In headless mode, skip window creation entirely to avoid GL init
  if (!headless) {
    // raylib::SetConfigFlags(raylib::FLAG_WINDOW_HIGHDPI);
    raylib::InitWindow(width, height, title);
    raylib::SetWindowSize(width, height);
    raylib::SetWindowState(raylib::FLAG_WINDOW_RESIZABLE);
  }
  if (!headless) {
    // Back to warnings
    raylib::TraceLogLevel logLevel = raylib::LOG_ERROR;
    raylib::SetTraceLogLevel(logLevel);
    raylib::SetTargetFPS(200);

    // Enlarge stream buffer to reduce dropouts on macOS/miniaudio
    raylib::SetAudioStreamBufferSizeDefault(4096);
    raylib::InitAudioDevice();
    if (!raylib::IsAudioDeviceReady()) {
      log_warn("audio device not ready; continuing without audio");
    }
    raylib::SetMasterVolume(1.f);

    // Disable default escape key exit behavior so we can handle it manually
    raylib::SetExitKey(0);
  }

  if (!headless) {
    load_gamepad_mappings();
    load_sounds();
  }

  // TODO add load folder for shaders

  auto load_shader = [](const char *file, const char *name) {
    std::string path_owned = Files::get().fetch_resource_path("shaders", file);
    const char *path = path_owned.c_str();
    ShaderLibrary::get().load(path, name);
  };
  if (!headless) {
    load_shader("post_processing.fs", "post_processing");
    load_shader("post_processing_tag.fs", "post_processing_tag");
    load_shader("text_mask.fs", "text_mask");
  }

  // TODO how safe is the path combination here esp for mac vs windows
  if (!headless)
    Files::get().for_resources_in_folder(
        "images", "controls/keyboard_default",
        [](const std::string &name, const std::string &filename) {
          TextureLibrary::get().load(filename.c_str(), name.c_str());
        });

  // TODO how safe is the path combination here esp for mac vs windows
  if (!headless)
    Files::get().for_resources_in_folder(
        "images", "controls/xbox_default",
        [](const std::string &name, const std::string &filename) {
          TextureLibrary::get().load(filename.c_str(), name.c_str());
        });

  // TODO add to spritesheet
  if (!headless) {
    TextureLibrary::get().load(
        Files::get().fetch_resource_path("images", "dollar_sign.png").c_str(),
        "dollar_sign");
    TextureLibrary::get().load(
        Files::get().fetch_resource_path("images", "trashcan.png").c_str(),
        "trashcan");
  }

  return *this;
}

void setup_fonts(Entity &sophie) {
  auto &font_manager = sophie.get<ui::FontManager>();

  font_manager.load_font(
      get_font_name(FontID::English),
      Files::get()
          .fetch_resource_path("", get_font_name(FontID::English))
          .c_str());

  font_manager.load_font(
      get_font_name(FontID::Korean),
      Files::get()
          .fetch_resource_path("", get_font_name(FontID::Korean))
          .c_str());

  font_manager.load_font(
      get_font_name(FontID::Japanese),
      Files::get()
          .fetch_resource_path("", get_font_name(FontID::Japanese))
          .c_str());

  std::string font_file =
      Files::get()
          .fetch_resource_path("", get_font_name(FontID::Korean))
          .c_str();

  translation_manager::TranslationManager::get().load_cjk_fonts(font_manager,
                                                                font_file);

  font_manager.load_font(
      afterhours::ui::UIComponent::SYMBOL_FONT,
      Files::get()
          .fetch_resource_path("", get_font_name(FontID::SYMBOL_FONT))
          .c_str());
}

Preload &Preload::make_singleton() {
  // sophie
  auto &sophie = EntityHelper::createEntity();
  {
    input::add_singleton_components(sophie, get_mapping());
    window_manager::add_singleton_components(sophie, 200);
    ui::add_singleton_components<InputAction>(sophie);

    auto &settings = Settings::get();
    translation_manager::set_language(settings.get_language());

    if (!render_backend::is_headless_mode) {
      texture_manager::add_singleton_components(
          sophie, raylib::LoadTexture(
                      Files::get()
                          .fetch_resource_path("images", "spritesheet.png")
                          .c_str()));
      setup_fonts(sophie);
    } else {
      // In headless mode, register texture manager without GPU textures
      texture_manager::add_singleton_components(sophie, {});
    }
    // making a root component to attach the UI to
    sophie.addComponent<ui::AutoLayoutRoot>();
    sophie.addComponent<ui::UIComponentDebug>("sophie");
    sophie.addComponent<ui::UIComponent>(sophie.id)
        .set_desired_width(afterhours::ui::screen_pct(1.f))
        .set_desired_height(afterhours::ui::screen_pct(1.f))
        .enable_font(get_active_font_name(), 75.f);

    // Navigation stack singleton for consistent UI navigation
    add_ui_singleton_components(sophie);
  }
  {
    // Audio emitter singleton for centralized sound requests
    auto &audio = EntityHelper::createEntity();
    audio.addComponent<SoundEmitter>();
    EntityHelper::registerSingleton<SoundEmitter>(audio);
  }
  {
    // Camera singleton for game world rendering
    auto &camera = EntityHelper::createEntity();
    auto &cameraComponent = camera.addComponent<HasCamera>();
    cameraComponent.camera.target = {0, 0};
    cameraComponent.camera.offset = {0, 0};
    EntityHelper::registerSingleton<HasCamera>(camera);
  }
  return *this;
}

Preload::~Preload() {
  if (raylib::IsAudioDeviceReady()) {
    // nothing to stop currently
    raylib::CloseAudioDevice();
  }
  if (raylib::IsWindowReady()) {
    raylib::CloseWindow();
  }
}