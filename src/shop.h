#pragma once

#include "rl.h"
#include <afterhours/ah.h>
#include <vector>

// Forward declaration for DishType
enum struct DishType;

struct Wallet : afterhours::BaseComponent {
  int gold = 10;
};

struct Health : afterhours::BaseComponent {
  int current = 5;
  int max = 5;
};

struct ShopState : afterhours::BaseComponent {
  bool initialized = false;
};

struct ShopTier : afterhours::BaseComponent {
  int current_tier = 1;
};

// Shop system constants
constexpr int SHOP_SLOTS = 7;
constexpr int INVENTORY_SLOTS = 7;
constexpr int SLOT_SIZE = 80;
constexpr int SLOT_GAP = 10;
constexpr int SHOP_START_X = 100;
constexpr int SHOP_START_Y = 200;
constexpr int INVENTORY_START_X = 100;
constexpr int INVENTORY_START_Y = 500;
constexpr int INVENTORY_SLOT_OFFSET = 100;

// Sell slot constants
constexpr int SELL_SLOT_ID = 999;
constexpr int SELL_SLOT_X = INVENTORY_START_X + (SLOT_SIZE + SLOT_GAP) * 8;
constexpr int SELL_SLOT_Y = INVENTORY_START_Y;

// Forward declarations for shop functions
afterhours::Entity &make_shop_manager(afterhours::Entity &);
afterhours::Entity &make_judging_manager(afterhours::Entity &);
afterhours::Entity &make_shop_item(int slot, DishType type);
afterhours::Entity &make_drop_slot(int slot_id, afterhours::vec2 position,
                                   afterhours::vec2 size,
                                   bool accepts_inventory = true,
                                   bool accepts_shop = true);
std::vector<int> get_free_slots(int max_slots);
DishType get_random_dish();
DishType get_random_dish_for_tier(int tier);
afterhours::vec2 calculate_slot_position(int slot, int start_x, int start_y,
                                         int cols = 4);
afterhours::vec2 calculate_inventory_position(int slot);

void register_shop_update_systems(afterhours::SystemManager &systems);
void register_shop_render_systems(afterhours::SystemManager &systems);

// Wallet helpers
bool wallet_can_afford(int cost);
bool wallet_charge(int cost);
bool charge_for_shop_purchase(DishType type);
