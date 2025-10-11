#pragma once

#include <afterhours/ah.h>
#include <string>
#include <vector>

struct Wallet : afterhours::BaseComponent {
  int gold = 10;
};

struct ShopState : afterhours::BaseComponent {
  struct Item {
    std::string name;
    int price = 3;
  };

  std::vector<Item> shop_items;
  std::vector<Item> inventory_items;
  bool initialized = false;
};

void make_shop_manager();
void register_shop_systems(afterhours::SystemManager &systems);
