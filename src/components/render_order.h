#pragma once

#include <afterhours/ah.h>

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

struct HasRenderOrder : afterhours::BaseComponent {
  RenderOrder order = RenderOrder::UI;

  HasRenderOrder() = default;
  explicit HasRenderOrder(RenderOrder o) : order(o) {}
};
