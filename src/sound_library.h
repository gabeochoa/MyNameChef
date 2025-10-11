#pragma once

#include <afterhours/src/library.h>
#include <afterhours/src/singleton.h>
//
#include "resources.h"

#include "rl.h"

enum struct SoundFile {
  UI_Select,
  UI_Move,
};

constexpr static const char *sound_file_to_str(SoundFile sf) {
  switch (sf) {
  case SoundFile::UI_Select:
    return "UISelect";
  case SoundFile::UI_Move:
    return "WaterDripSingle";
  }
  return "";
}

SINGLETON_FWD(SoundLibrary)
struct SoundLibrary {
  SINGLETON(SoundLibrary)

  [[nodiscard]] raylib::Sound &get(const std::string &name) {
    return impl.get(name);
  }
  [[nodiscard]] const raylib::Sound &get(const std::string &name) const {
    return impl.get(name);
  }
  [[nodiscard]] bool contains(const std::string &name) const {
    return impl.contains(name);
  }
  void load(const char *filename, const char *name) {
    impl.load(filename, name);
  }

  void play(SoundFile file) { play(sound_file_to_str(file)); }
  void play(const char *const name) { PlaySound(get(name)); }

  void play_random_match(const std::string &prefix) {
    impl.get_random_match(prefix).transform(raylib::PlaySound);
  }

  void play_if_none_playing(const std::string &prefix) {
    auto matches = impl.lookup(prefix);
    if (matches.first == matches.second) {
      log_warn("got no matches for your prefix search: {}", prefix);
      return;
    }
    for (auto it = matches.first; it != matches.second; ++it) {
      if (raylib::IsSoundPlaying(it->second)) {
        return;
      }
    }
    raylib::PlaySound(matches.first->second);
  }

  void play_first_available_match(const std::string &prefix) {
    auto matches = impl.lookup(prefix);
    if (matches.first == matches.second) {
      log_warn("got no matches for your prefix search: {}", prefix);
      return;
    }
    for (auto it = matches.first; it != matches.second; ++it) {
      if (!raylib::IsSoundPlaying(it->second)) {
        raylib::PlaySound(it->second);
        return;
      }
    }
    raylib::PlaySound(matches.first->second);
  }

  void update_volume(const float new_v) {
    impl.update_volume(new_v);
    current_volume = new_v;
  }

  void unload_all() { impl.unload_all(); }

private:
  // Note: Read note in MusicLibrary
  float current_volume = 1.f;

  struct SoundLibraryImpl : Library<raylib::Sound> {
    virtual raylib::Sound
    convert_filename_to_object(const char *, const char *filename) override {
      return raylib::LoadSound(filename);
    }
    virtual void unload(raylib::Sound sound) override {
      raylib::UnloadSound(sound);
    }

    void update_volume(const float new_v) {
      for (const auto &kv : storage) {
        log_trace("updating sound volume for {} to {}", kv.first, new_v);
        raylib::SetSoundVolume(kv.second, new_v);
      }
    }
  } impl;
};

constexpr static void load_sounds() {
  magic_enum::enum_for_each<SoundFile>([](auto val) {
    constexpr SoundFile file = val;
    std::string filename;
    switch (file) {
    case SoundFile::UI_Select:
      filename = "gdc/doex_qantum_ui_ui_select_plastic_05_03.wav";
      break;
    case SoundFile::UI_Move:
      filename = "gdc/"
                 "inmotionaudio_cave_design_WATRDrip_SingleDrip03_"
                 "InMotionAudio_CaveDesign.wav";
      break;
    default:
      // Skip loading sounds that don't have files available
      return;
    }
    SoundLibrary::get().load(
        Files::get().fetch_resource_path("sounds", filename).c_str(),
        sound_file_to_str(file));
  });
}
