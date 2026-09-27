#include "preload.h"

#include <iostream>
#include <sstream>
#include <vector>

#include "log.h"
#include "render_backend.h"
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

  // In plain headless mode, skip window creation entirely to avoid GL init.
  // In offscreen-render mode, create a hidden window: a real GL context
  // (textures, shaders, render textures all work) with nothing on screen.
  const bool offscreen = render_backend::is_offscreen_render_mode;
  if (!headless) {
    // raylib::SetConfigFlags(raylib::FLAG_WINDOW_HIGHDPI);
    raylib::InitWindow(width, height, title);
    raylib::SetWindowSize(width, height);
    raylib::SetWindowState(raylib::FLAG_WINDOW_RESIZABLE);
  } else if (offscreen) {
    raylib::SetConfigFlags(raylib::FLAG_WINDOW_HIDDEN);
    raylib::InitWindow(width, height, title);
    raylib::SetTraceLogLevel(raylib::LOG_ERROR);
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
  if (!headless || offscreen) {
    load_shader("post_processing.fs", "post_processing");
    load_shader("post_processing_tag.fs", "post_processing_tag");
    load_shader("text_mask.fs", "text_mask");
  }

  // TODO how safe is the path combination here esp for mac vs windows
  if (!headless || offscreen)
    Files::get().for_resources_in_folder(
        "images", "controls/keyboard_default",
        [](const std::string &name, const std::string &filename) {
          TextureLibrary::get().load(filename.c_str(), name.c_str());
        });

  // TODO how safe is the path combination here esp for mac vs windows
  if (!headless || offscreen)
    Files::get().for_resources_in_folder(
        "images", "controls/xbox_default",
        [](const std::string &name, const std::string &filename) {
          TextureLibrary::get().load(filename.c_str(), name.c_str());
        });

  // TODO add to spritesheet
  if (!headless || offscreen) {
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

void setup_headless_fonts(Entity &sophie) {
  auto &font_manager = sophie.get<ui::FontManager>();
  raylib::Font font = raylib::GetFontDefault();
  for (FontID id : {FontID::English, FontID::Korean, FontID::Japanese,
                    FontID::SYMBOL_FONT}) {
    font_manager.load_font(get_font_name(id), font);
  }
  sophie.get<ui::TextMeasureCache>().set_measure_function(
      [](std::string_view text, std::string_view, float font_size,
         float spacing) {
        float width = static_cast<float>(text.size()) *
                      (font_size * 0.6f + spacing);
        return raylib::Vector2{width, font_size};
      });
}

Preload &Preload::make_singleton() {
  auto &sophie = EntityHelper::createEntity();
  {
    input::add_singleton_components(sophie, get_mapping());
    window_manager::add_singleton_components(sophie, 200);
    Entity &ui_root = ui::init_ui_plugin<InputAction>();
    translation_manager::initialize_translation_plugin(sophie);

    auto &settings = Settings::get();
    translation_manager::set_language(settings.get_language());

    if (render_backend::should_render()) {
      texture_manager::add_singleton_components(
          sophie, raylib::LoadTexture(
                      Files::get()
                          .fetch_resource_path("images", "spritesheet.png")
                          .c_str()));
      setup_fonts(ui_root);
    } else {
      texture_manager::add_singleton_components(sophie, {});
      setup_headless_fonts(ui_root);
    }
    add_ui_singleton_components(ui_root);
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