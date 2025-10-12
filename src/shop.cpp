#include "shop.h"
#include "components/is_dish.h"
#include "components/is_shop_item.h"
#include "components/transform.h"
#include "game_state_manager.h"
#include "input_mapping.h"
#include "ui/containers.h"
#include "ui/controls.h"
#include "ui/metrics.h"
#include "ui/ui_systems.h"
#include <afterhours/src/plugins/color.h>
#include <memory>

using namespace afterhours;

struct ShopItemColor : HasColor {
  ShopItemColor(raylib::Color color) : HasColor(color) {}
};

void make_shop_manager(Entity &sophie) {
  sophie.addComponent<Wallet>();
  sophie.addComponent<ShopState>();
  EntityHelper::registerSingleton<Wallet>(sophie);
  EntityHelper::registerSingleton<ShopState>(sophie);
}

Entity &make_shop_item(int slot, const char *name, raylib::Color color) {
  auto &e = EntityHelper::createEntity();

  // Position items in a grid layout
  int itemW = 80, itemH = 80, startX = 100, startY = 200, gap = 10;
  float x = startX + (slot % 4) * (itemW + gap);
  float y = startY + (slot / 4) * (itemH + gap);

  e.addComponent<Transform>(vec2{x, y}, vec2{(float)itemW, (float)itemH});
  e.addComponent<IsDish>(name, color);
  e.addComponent<IsShopItem>(slot);
  e.addComponent<ShopItemColor>(color);

  return e;
}

static constexpr std::pair<const char *, raylib::Color> dish_pool[] = {
    {"Garlic Bread", raylib::Color{180, 120, 80, 255}},
    {"Tomato Soup", raylib::Color{160, 40, 40, 255}},
    {"Grilled Cheese", raylib::Color{200, 160, 60, 255}},
    {"Chicken Skewer", raylib::Color{150, 80, 60, 255}},
    {"Cucumber Salad", raylib::Color{60, 160, 100, 255}},
    {"Vanilla Soft Serve", raylib::Color{220, 200, 180, 255}},
    {"Caprese Salad", raylib::Color{120, 200, 140, 255}},
    {"Minestrone", raylib::Color{160, 90, 60, 255}},
    {"Seared Salmon", raylib::Color{230, 140, 90, 255}},
    {"Steak Florentine", raylib::Color{160, 80, 80, 255}},
};

struct ShopGenerationSystem : System<> {
  bool initialized = false;

  void once(float) override {
    if (initialized)
      return;
    initialized = true;

    // Determine occupied slots
    std::array<bool, 7> occupied{};
    occupied.fill(false);
    for (auto &ref : EntityQuery().whereHasComponent<IsShopItem>().gen()) {
      auto &ent = ref.get();
      int s = std::clamp(ent.get<IsShopItem>().slot, 0, 6);
      occupied[(size_t)s] = true;
    }
    // Free slots
    std::vector<int> free_slots;
    free_slots.reserve(7);
    for (int s = 0; s < 7; ++s)
      if (!occupied[(size_t)s])
        free_slots.push_back(s);
    if (free_slots.empty()) {
      return;
    }

    std::vector<int> idx(sizeof(dish_pool) / sizeof(dish_pool[0]));
    std::iota(idx.begin(), idx.end(), 0);
    std::shuffle(idx.begin(), idx.end(), std::mt19937{std::random_device{}()});

    int take = std::min((int)free_slots.size(), (int)idx.size());
    for (int i = 0; i < take; ++i) {
      auto [nm, col] = dish_pool[(size_t)idx[(size_t)i]];
      int slot = free_slots[(size_t)i];
      auto &entity = make_shop_item(slot, nm, col);
    }

    // Merge temp entities into main entity list so they're visible to queries
    EntityHelper::merge_entity_arrays();
  }
};

void register_shop_update_systems(afterhours::SystemManager &systems) {
  systems.register_update_system(std::make_unique<ShopGenerationSystem>());
}

void register_shop_render_systems(afterhours::SystemManager &systems) {}
