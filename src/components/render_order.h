#pragma once

#include <afterhours/ah.h>
#include <vector>

enum struct RenderOrder : int {
  Background = 0,
  DropSlots = 100,
  InventoryItems = 200,
  ShopItems = 300,
  BattleTeams = 400,
  UI = 500,
  Tooltips = 600,
  Debug = 700,
};

enum struct RenderScreen : int {
  Shop = 1,
  Battle = 2,
  Results = 4,
  All = Shop | Battle | Results,
};

struct HasRenderOrder : afterhours::BaseComponent {
  RenderOrder order = RenderOrder::UI;
  RenderScreen screens = RenderScreen::All; // Default to all screens

  HasRenderOrder() = default;
  explicit HasRenderOrder(RenderOrder o) : order(o) {}
  HasRenderOrder(RenderOrder o, RenderScreen s) : order(o), screens(s) {}
  
  // Helper method to check if entity should render on current screen
  bool should_render_on_screen(RenderScreen current_screen) const {
    return (static_cast<int>(screens) & static_cast<int>(current_screen)) != 0;
  }
};
