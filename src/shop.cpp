#include "shop.h"
#include "components/animation_event.h"
#include "components/battle_processor.h"
#include "components/battle_session_registry.h"
#include "components/can_drop_onto.h"
#include "components/combat_queue.h"
#include "components/dish_level.h"
#include "components/has_tooltip.h"
#include "components/is_dish.h"
#include "components/is_draggable.h"
#include "components/is_drop_slot.h"
#include "components/is_shop_item.h"
#include "components/render_order.h"
#include "components/replay_state.h"
#include "components/side_effect_tracker.h"
#include "components/transform.h"
#include "components/trigger_queue.h"
#include "dish_types.h"
#include "game_state_manager.h"
#include "render_backend.h"
#include "render_constants.h"
#include "systems/GenerateInventorySlots.h"
#include "systems/GenerateShopSlots.h"
#include "tooltip.h"
#include <afterhours/src/plugins/color.h>
#include <afterhours/src/plugins/texture_manager.h>
#include <magic_enum/magic_enum.hpp>
#include <memory>
#include <random>

using namespace afterhours;

vec2 calculate_slot_position(int slot, int start_x, int start_y, int cols) {
  int col = slot % cols;
  int row = slot / cols;
  float x = static_cast<float>(start_x + col * (SLOT_SIZE + SLOT_GAP));
  float y = static_cast<float>(start_y + row * (SLOT_SIZE + SLOT_GAP));
  return vec2{x, y};
}

vec2 calculate_inventory_position(int slot) {
  float x = INVENTORY_START_X + slot * (SLOT_SIZE + SLOT_GAP);
  return vec2{x, static_cast<float>(INVENTORY_START_Y)};
}

std::vector<int> get_free_slots(int max_slots) {
  std::vector<int> free_slots;
  free_slots.reserve(max_slots);

  for (int i = 0; i < max_slots; ++i) {
    bool occupied = false;

    for (auto &ref : EntityQuery({.force_merge = true})
                         .whereHasComponent<IsShopItem>()
                         .gen()) {
      if (ref.get().get<IsShopItem>().slot == i) {
        occupied = true;
        break;
      }
    }

    if (!occupied) {
      free_slots.push_back(i);
    }
  }

  return free_slots;
}

DishType get_random_dish() {
  static std::random_device rd;
  static std::mt19937 gen(rd());
  const auto &pool = get_default_dish_pool();
  static std::uniform_int_distribution<> dis(0, (int)pool.size() - 1);
  return pool[dis(gen)];
}

DishType get_random_dish_for_tier(int tier) {
  static std::random_device rd;
  static std::mt19937 gen(rd());
  const auto &pool = get_dishes_for_tier(tier);
  if (pool.empty()) {
    log_warn("No dishes available for tier {}, falling back to potato", tier);
    return DishType::Potato;
  }
  static std::uniform_int_distribution<> dis(0, (int)pool.size() - 1);
  return pool[dis(gen)];
}

Entity &make_shop_manager(Entity &sophie) {
  sophie.addComponent<Wallet>();
  sophie.addComponent<Health>();
  sophie.addComponent<ShopState>();
  sophie.addComponent<ShopTier>();
  EntityHelper::registerSingleton<Wallet>(sophie);
  EntityHelper::registerSingleton<Health>(sophie);
  EntityHelper::registerSingleton<ShopState>(sophie);
  EntityHelper::registerSingleton<ShopTier>(sophie);
  return sophie;
}

Entity &make_combat_manager(Entity &sophie) {
  sophie.addComponentIfMissing<CombatQueue>();
  EntityHelper::registerSingleton<CombatQueue>(sophie);
  return sophie;
}

Entity &make_battle_processor_manager(Entity &sophie) {
  sophie.addComponentIfMissing<BattleProcessor>();
  EntityHelper::registerSingleton<BattleProcessor>(sophie);
  sophie.addComponentIfMissing<TriggerQueue>();
  EntityHelper::registerSingleton<TriggerQueue>(sophie);
  sophie.addComponentIfMissing<BattleSessionRegistry>();
  EntityHelper::registerSingleton<BattleSessionRegistry>(sophie);
  sophie.addComponentIfMissing<ReplayState>();
  EntityHelper::registerSingleton<ReplayState>(sophie);
  sophie.addComponentIfMissing<SideEffectTracker>();
  EntityHelper::registerSingleton<SideEffectTracker>(sophie);
  return sophie;
}

Entity &make_shop_item(int slot, DishType type) {
  auto &e = EntityHelper::createEntity();
  auto position = calculate_slot_position(slot, SHOP_START_X, SHOP_START_Y);
  auto dish_info = get_dish_info(type);

  e.addComponent<Transform>(position, vec2{SLOT_SIZE, SLOT_SIZE});
  e.addComponent<IsDish>(type);
  e.addComponent<DishLevel>(1); // Start at level 1
  e.addComponent<IsShopItem>(slot);
  e.addComponent<IsDraggable>(true);
  e.addComponent<HasRenderOrder>(RenderOrder::ShopItems, RenderScreen::Shop);
  // Attach sprite using dish atlas grid indices
  {
    const auto frame = afterhours::texture_manager::idx_to_sprite_frame(
        dish_info.sprite.i, dish_info.sprite.j);
    // Position and size are updated each frame by UpdateSpriteTransform
    e.addComponent<afterhours::texture_manager::HasSprite>(
        position, vec2{SLOT_SIZE, SLOT_SIZE}, 0.f, frame,
        render_constants::kDishSpriteScale, raylib::WHITE);
  }
  e.addComponent<HasTooltip>(generate_dish_tooltip(type));

  return e;
}

Entity &make_drop_slot(int slot_id, vec2 position, vec2 size,
                       bool accepts_inventory, bool accepts_shop) {
  auto &e = EntityHelper::createEntity();

  e.addComponent<Transform>(position, size);
  e.addComponent<IsDropSlot>(slot_id, accepts_inventory, accepts_shop);
  e.addComponent<CanDropOnto>(true);
  e.addComponent<HasRenderOrder>(RenderOrder::DropSlots, RenderScreen::Shop);

  return e;
}

struct ScreenTransitionSystem : System<> {
  void for_each_with(Entity &, float) override {
    GameStateManager::get().update_screen();
  }
};

void register_shop_update_systems(afterhours::SystemManager &systems) {
  systems.register_update_system(std::make_unique<GenerateShopSlots>());
  systems.register_update_system(std::make_unique<GenerateInventorySlots>());
  systems.register_update_system(std::make_unique<ScreenTransitionSystem>());
}

void register_shop_render_systems(afterhours::SystemManager &) {}

bool wallet_can_afford(int cost) {
  auto wallet_entity = EntityHelper::get_singleton<Wallet>();
  if (!wallet_entity.get().has<Wallet>())
    return false;
  auto &wallet = wallet_entity.get().get<Wallet>();
  return wallet.gold >= cost;
}

bool wallet_charge(int cost) {
  auto wallet_entity = EntityHelper::get_singleton<Wallet>();
  if (!wallet_entity.get().has<Wallet>())
    return false;
  auto &wallet = wallet_entity.get().get<Wallet>();
  if (wallet.gold < cost)
    return false;
  wallet.gold -= cost;
  return true;
}

bool charge_for_shop_purchase(DishType type) {
  const int price = get_dish_info(type).price;
  return wallet_charge(price);
}

bool hasActiveAnimation() {
  // In headless mode, animations complete instantly, so there are never any blocking animations
  if (render_backend::is_headless_mode) {
    return false;
  }
  return EntityQuery()
      .whereHasComponent<IsBlockingAnimationEvent>()
      .has_values();
}

bool isReplayPaused() {
  auto replayState = EntityHelper::get_singleton<ReplayState>();
  if (!replayState.get().has<ReplayState>()) {
    return false;
  }
  const ReplayState &rs = replayState.get().get<ReplayState>();
  return rs.active && rs.paused;
}

afterhours::Entity &make_animation_event(AnimationEventType type,
                                         bool blocking) {
  auto &e = EntityHelper::createEntity();
  e.addComponent<AnimationEvent>();
  e.get<AnimationEvent>().type = type;
  if (blocking) {
    e.addComponent<IsBlockingAnimationEvent>();
  }
  return e;
}

afterhours::Entity &make_freshness_chain_animation(int sourceEntityId,
                                                   int previousEntityId,
                                                   int nextEntityId) {
  auto &e = make_animation_event(AnimationEventType::FreshnessChain, true);
  auto &animEvent = e.get<AnimationEvent>();
  animEvent.data =
      FreshnessChainData{sourceEntityId, previousEntityId, nextEntityId};
  return e;
}
