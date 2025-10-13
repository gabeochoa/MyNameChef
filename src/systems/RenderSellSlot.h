#pragma once

#include "../components/is_drop_slot.h"
#include "../components/transform.h"
#include "../game_state_manager.h"
#include "../shop.h"
#include "../texture_library.h"
#include <afterhours/ah.h>

struct RenderSellSlot : afterhours::System<> {
  virtual bool should_run(float) override {
    auto &gsm = GameStateManager::get();
    return gsm.active_screen == GameStateManager::Screen::Shop;
  }

  void once(float) const override {
    // Find the sell slot and draw the trash icon centered within it
    for (auto &ref : afterhours::EntityQuery()
                         .template whereHasComponent<IsDropSlot>()
                         .template whereHasComponent<Transform>()
                         .gen()) {
      const auto &e = ref.get();
      const auto &slot = e.get<IsDropSlot>();
      if (slot.slot_id != SELL_SLOT_ID)
        continue;

      const auto &t = e.get<Transform>();
      const auto &tex = TextureLibrary::get().get("trashcan");

      // Scale the icon to fit nicely within the slot
      const float scale = 0.6f;
      const float dest_w = t.size.x * scale;
      const float dest_h = t.size.y * scale;
      const float dest_x = t.position.x + (t.size.x - dest_w) * 0.5f;
      const float dest_y = t.position.y + (t.size.y - dest_h) * 0.5f;

      raylib::DrawTexturePro(tex,
                             raylib::Rectangle{0, 0,
                                               static_cast<float>(tex.width),
                                               static_cast<float>(tex.height)},
                             raylib::Rectangle{dest_x, dest_y, dest_w, dest_h},
                             afterhours::vec2{0, 0}, 0.f, raylib::WHITE);
      break;
    }
  }
};
