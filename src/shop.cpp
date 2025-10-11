#include "shop.h"
#include "game_state_manager.h"
#include "input_mapping.h"
#include "ui/containers.h"
#include "ui/controls.h"
#include "ui/metrics.h"
#include "ui/ui_systems.h"
#include <memory>

using namespace afterhours;

void make_shop_manager() {
  Entity &e = EntityHelper::createEntity();
  e.addComponent<Wallet>();
  e.addComponent<ShopState>();
  EntityHelper::registerSingleton<Wallet>(e);
  EntityHelper::registerSingleton<ShopState>(e);
}

struct ShopGenerationSystem : System<ShopState> {
  virtual bool should_run(float) override {
    return GameStateManager::get().is_game_active();
  }
  void for_each_with(Entity &, ShopState &shop, float) override {
    if (shop.initialized)
      return;
    shop.initialized = true;
    shop.shop_items = {
        {"Garlic Bread", 3},   {"Tomato Soup", 3},    {"Grilled Cheese", 3},
        {"Chicken Skewer", 3}, {"Cucumber Salad", 3}, {"Vanilla Soft Serve", 3},
    };
  }
};

struct ShopUISystem : System<ui::UIContext<InputAction>> {
  virtual bool should_run(float) override {
    return GameStateManager::get().is_game_active();
  }

  void for_each_with(Entity &entity, ui::UIContext<InputAction> &context,
                     float) override {
    using namespace afterhours::ui;
    using namespace afterhours::ui::imm;
    using namespace afterhours::ui::controls;
    using namespace afterhours::ui::containers;

    auto *wallet = EntityHelper::get_singleton_cmp<Wallet>();
    auto *shop = EntityHelper::get_singleton_cmp<ShopState>();
    if (!wallet || !shop)
      return;

    // Root container
    auto root = imm::div(context, mk(entity),
                         imm::ComponentConfig{}
                             .with_size(ui::ComponentSize{ui::screen_pct(1.f),
                                                          ui::screen_pct(1.f)})
                             .with_debug_name("shop_root")
                             .with_absolute_position());

    auto left = column_left<InputAction>(context, root.ent(), "shop_left", 0);
    auto right =
        column_right<InputAction>(context, root.ent(), "shop_right", 1);

    // Wallet display
    imm::div(context, mk(left.ent(), 0),
             imm::ComponentConfig{}
                 .with_label("Gold: " + std::to_string(wallet->gold))
                 .with_debug_name("wallet_label"));

    // Shop items list with Buy buttons
    int idx = 1;
    for (size_t i = 0; i < shop->shop_items.size(); ++i) {
      const auto &item = shop->shop_items[i];
      std::string label = item.name + " ($" + std::to_string(item.price) + ")";
      if (imm::button(context, mk(left.ent(), idx++),
                      imm::ComponentConfig{}.with_label("Buy: " + label))) {
        if (wallet->gold >= item.price) {
          wallet->gold -= item.price;
          shop->inventory_items.push_back(item);
        }
      }
    }

    // Inventory list with Sell buttons
    imm::div(context, mk(right.ent(), 0),
             imm::ComponentConfig{}.with_label("Inventory:"));
    int ridx = 1;
    for (size_t i = 0; i < shop->inventory_items.size(); ++i) {
      const auto &inv = shop->inventory_items[i];
      if (imm::button(context, mk(right.ent(), ridx++),
                      imm::ComponentConfig{}.with_label("Sell: " + inv.name))) {
        wallet->gold += inv.price; // 100% refund for Step 1 simplicity
        // remove one instance
        shop->inventory_items.erase(shop->inventory_items.begin() +
                                    static_cast<long>(i));
        --i;
      }
    }
  }
};

void register_shop_systems(SystemManager &systems) {
  systems.register_update_system(std::make_unique<ShopGenerationSystem>());
  systems.register_update_system(std::make_unique<ShopUISystem>());
}
