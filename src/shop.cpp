#include "shop.h"
#include "components/can_drop_onto.h"
#include "components/has_tooltip.h"
#include "components/is_dish.h"
#include "components/is_draggable.h"
#include "components/is_drop_slot.h"
#include "components/is_shop_item.h"
#include "components/transform.h"
#include "game_state_manager.h"
#include <afterhours/src/plugins/color.h>
#include <memory>

using namespace afterhours;

struct ShopItemColor : HasColor {
  ShopItemColor(raylib::Color color) : HasColor(color) {}
};

void make_shop_manager(Entity &sophie) {
  sophie.addComponent<Wallet>();
  sophie.addComponent<Health>();
  sophie.addComponent<ShopState>();
  EntityHelper::registerSingleton<Wallet>(sophie);
  EntityHelper::registerSingleton<Health>(sophie);
  EntityHelper::registerSingleton<ShopState>(sophie);
}

Entity &make_shop_item(int slot, DishType type) {
  auto &e = EntityHelper::createEntity();

  // Position items in a grid layout
  int itemW = 80, itemH = 80, startX = 100, startY = 200, gap = 10;
  float x = startX + (slot % 4) * (itemW + gap);
  float y = startY + (slot / 4) * (itemH + gap);

  e.addComponent<Transform>(vec2{x, y}, vec2{(float)itemW, (float)itemH});
  e.addComponent<IsDish>(type);
  e.addComponent<IsShopItem>(slot);
  e.addComponent<IsDraggable>(true);

  // Get dish info once to avoid multiple calls
  auto dish_info = get_dish_info(type);
  e.addComponent<ShopItemColor>(dish_info.color);

  // Add tooltip with dish information
  std::string tooltip_text =
      dish_info.name + "\n[GOLD]Price: " + std::to_string(dish_info.price) +
      " gold";

  // Add flavor stats (only show non-zero values) with colors
  std::vector<std::pair<std::string, int>> flavor_stats = {
      {"[YELLOW]Sweetness", dish_info.flavor.sweetness},
      {"[RED]Spice", dish_info.flavor.spice},
      {"[GREEN]Acidity", dish_info.flavor.acidity},
      {"[BLUE]Umami", dish_info.flavor.umami},
      {"[ORANGE]Richness", dish_info.flavor.richness},
      {"[CYAN]Freshness", dish_info.flavor.freshness},
      {"[PURPLE]Satiety", dish_info.flavor.satiety}};

  for (const auto &[name, value] : flavor_stats) {
    if (value > 0) {
      tooltip_text += "\n" + name + ": " + std::to_string(value);
    }
  }
  e.addComponent<HasTooltip>(tooltip_text);

  return e;
}

Entity &make_drop_slot(int slot_id, vec2 position, vec2 size,
                       bool accepts_inventory = true,
                       bool accepts_shop = true) {
  auto &e = EntityHelper::createEntity();

  e.addComponent<Transform>(position, size);
  e.addComponent<IsDropSlot>(slot_id, accepts_inventory, accepts_shop);
  e.addComponent<CanDropOnto>(true);

  return e;
}

namespace {
constexpr DishType dish_pool[] = {
    DishType::GarlicBread,   DishType::TomatoSoup,
    DishType::GrilledCheese, DishType::ChickenSkewer,
    DishType::CucumberSalad, DishType::VanillaSoftServe,
    DishType::CapreseSalad,  DishType::Minestrone,
    DishType::SearedSalmon,  DishType::SteakFlorentine,
};
}

struct ShopGenerationSystem : System<> {
  bool initialized = false;

  void once(float) override {
    if (initialized)
      return;
    initialized = true;

    // Create drop slots FIRST so they render behind items
    int slotW = 80, slotH = 80, startX = 100, startY = 200, gap = 10;
    for (int i = 0; i < 7; ++i) {
      float x = startX + (i % 4) * (slotW + gap);
      float y = startY + (i / 4) * (slotH + gap);
      make_drop_slot(i, vec2{x, y}, vec2{(float)slotW, (float)slotH}, false,
                     true);
    }

    // Create inventory drop slots (example: at the bottom of the screen)
    int invStartX = 100, invStartY = 500, invSlotW = 80, invSlotH = 80,
        invGap = 10;
    for (int i = 0; i < 7; ++i) {
      float x = invStartX + i * (invSlotW + invGap);
      float y = invStartY;
      make_drop_slot(100 + i, vec2{x, y},
                     vec2{(float)invSlotW, (float)invSlotH}, true, true);
    }

    // Merge temp entities so we can query for existing shop items
    EntityHelper::merge_entity_arrays();

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
      auto dish_type = dish_pool[(size_t)idx[(size_t)i]];
      int slot = free_slots[(size_t)i];
      make_shop_item(slot, dish_type);
    }

    // Merge temp entities so we can query for slots
    EntityHelper::merge_entity_arrays();

    // Mark slots as occupied for items that were just created
    for (int i = 0; i < take; ++i) {
      int slot = free_slots[(size_t)i];
      for (auto &ref : EntityQuery().whereHasComponent<IsDropSlot>().gen()) {
        auto &slot_entity = ref.get();
        if (slot_entity.get<IsDropSlot>().slot_id == slot) {
          slot_entity.get<IsDropSlot>().occupied = true;
          break;
        }
      }
    }
  }
};

struct ScreenTransitionSystem : System<> {
  void for_each_with(Entity &, float) override {
    GameStateManager::get().update_screen();
  }
};

void register_shop_update_systems(afterhours::SystemManager &systems) {
  systems.register_update_system(std::make_unique<ShopGenerationSystem>());
  systems.register_update_system(std::make_unique<ScreenTransitionSystem>());
}

void register_shop_render_systems(afterhours::SystemManager &) {}
