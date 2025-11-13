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
#include "components/network_info.h"
#include "components/render_order.h"
#include "components/replay_state.h"
#include "components/side_effect_tracker.h"
#include "components/synergy_counts.h"
#include "components/toast_message.h"
#include "components/transform.h"
#include "components/trigger_queue.h"
#include "components/user_id.h"
#include "dish_types.h"
#include "game_state_manager.h"
#include "log.h"
#include "render_backend.h"
#include "render_constants.h"
#include "seeded_rng.h"
#include "systems/GenerateInventorySlots.h"
#include "systems/GenerateShopSlots.h"
#include "systems/NetworkSystem.h"
#include "systems/SynergyCountingSystem.h"
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
  auto &rng = SeededRng::get();
  const auto &pool = get_default_dish_pool();
  return pool[rng.gen_index(pool.size())];
}

DishType get_random_dish_for_tier(int tier) {
  auto &rng = SeededRng::get();
  const auto &pool = get_dishes_for_tier(tier);
  if (pool.empty()) {
    log_warn("No dishes available for tier {}, falling back to potato", tier);
    return DishType::Potato;
  }
  return pool[rng.gen_index(pool.size())];
}

Entity &make_shop_manager(Entity &sophie) {
  sophie.addComponentIfMissing<Wallet>();
  sophie.addComponentIfMissing<Health>();
  sophie.addComponentIfMissing<ShopState>();
  sophie.addComponentIfMissing<ShopTier>();
  sophie.addComponentIfMissing<RerollCost>(
      1, 0); // base=1, increment=0 (cost stays 1 initially)
  sophie.addComponentIfMissing<SynergyCounts>();

  // Register singletons only if not already registered
  // This prevents overwriting existing singletons with new entities/components
  using namespace afterhours::components;
  auto &singleton_map = EntityHelper::get().singletonMap;

  const auto wallet_id = get_type_id<Wallet>();
  if (singleton_map.find(wallet_id) == singleton_map.end()) {
    EntityHelper::registerSingleton<Wallet>(sophie);
  } else {
    // If already registered, ensure this entity has the component too
    // (so make_shop_manager can safely be called multiple times on same entity)
    sophie.addComponentIfMissing<Wallet>();
  }

  const auto health_id = get_type_id<Health>();
  if (singleton_map.find(health_id) == singleton_map.end()) {
    EntityHelper::registerSingleton<Health>(sophie);
  } else {
    sophie.addComponentIfMissing<Health>();
  }

  const auto shop_state_id = get_type_id<ShopState>();
  if (singleton_map.find(shop_state_id) == singleton_map.end()) {
    EntityHelper::registerSingleton<ShopState>(sophie);
  } else {
    sophie.addComponentIfMissing<ShopState>();
  }

  const auto shop_tier_id = get_type_id<ShopTier>();
  if (singleton_map.find(shop_tier_id) == singleton_map.end()) {
    EntityHelper::registerSingleton<ShopTier>(sophie);
  } else {
    sophie.addComponentIfMissing<ShopTier>();
  }

  const auto reroll_cost_id = get_type_id<RerollCost>();
  if (singleton_map.find(reroll_cost_id) == singleton_map.end()) {
    EntityHelper::registerSingleton<RerollCost>(sophie);
  } else {
    sophie.addComponentIfMissing<RerollCost>(1, 0);
  }

  const auto synergy_counts_id = get_type_id<SynergyCounts>();
  if (singleton_map.find(synergy_counts_id) == singleton_map.end()) {
    EntityHelper::registerSingleton<SynergyCounts>(sophie);
  } else {
    sophie.addComponentIfMissing<SynergyCounts>();
  }
  return sophie;
}

Entity &make_network_manager(Entity &sophie) {
  sophie.addComponentIfMissing<NetworkInfo>();
  EntityHelper::registerSingleton<NetworkInfo>(sophie);

  // Perform initial health check to establish connection state before systems
  // run
  NetworkInfo &networkInfo = sophie.get<NetworkInfo>();
  ServerAddress addr;
  addr.ip = NetworkSystem::SERVER_IP;
  addr.port = NetworkSystem::SERVER_PORT;
  networkInfo.serverAddress = addr;

  bool connected = NetworkSystem::check_server_health(addr);
  networkInfo.hasConnection = connected;
  if (connected) {
    log_info("NETWORK: Initial connection check - Server connection OK");
  } else {
    log_warn("NETWORK: Initial connection check - Server connection failed");
  }

  // Initialize countdown timer to check_interval
  float check_interval = NetworkSystem::get_check_interval();
  networkInfo.timeSinceLastCheck = check_interval;

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

Entity &make_user_id_singleton() {
  auto &userId_entity = EntityHelper::createEntity();
  userId_entity.addComponent<UserId>();
  EntityHelper::registerSingleton<UserId>(userId_entity);
  log_info("Initialized UserId: {}", userId_entity.get<UserId>().userId);
  return userId_entity;
}

Entity &make_round_singleton(Entity &sophie) {
  sophie.addComponentIfMissing<Round>();
  EntityHelper::registerSingleton<Round>(sophie);
  return sophie;
}

Entity &make_continue_game_request_singleton(Entity &sophie) {
  sophie.addComponentIfMissing<ContinueGameRequest>();
  EntityHelper::registerSingleton<ContinueGameRequest>(sophie);
  return sophie;
}

Entity &make_continue_button_disabled_singleton(Entity &sophie) {
  sophie.addComponentIfMissing<ContinueButtonDisabled>();
  EntityHelper::registerSingleton<ContinueButtonDisabled>(sophie);
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
  e.addComponent<Freezeable>(false);
  e.addComponent<HasRenderOrder>(RenderOrder::ShopItems, RenderScreen::Shop);
  add_dish_tags(e, type);
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

Entity &make_toast(const std::string &message, float duration) {
  auto &e = EntityHelper::createEntity();

  const float fontSize = 20.0f;
  const float paddingX = 20.0f;
  const float paddingY = 10.0f;

  float textWidth =
      raylib::MeasureText(message.c_str(), static_cast<int>(fontSize));
  float toastWidth = textWidth + paddingX * 2.0f;
  float toastHeight = fontSize + paddingY * 2.0f;

  float screenWidth = 800.0f;
  float screenHeight = 600.0f;
  if (!render_backend::is_headless_mode) {
    screenWidth = static_cast<float>(raylib::GetScreenWidth());
    screenHeight = static_cast<float>(raylib::GetScreenHeight());
  }

  float startY = screenHeight + 50.0f;
  float x = (screenWidth - toastWidth) / 2.0f;

  e.addComponent<Transform>(vec2{x, startY}, vec2{toastWidth, toastHeight});
  e.addComponent<HasColor>(raylib::Color{40, 40, 40, 0});
  e.addComponent<ToastMessage>(message, duration);
  e.addComponent<HasRenderOrder>(RenderOrder::UI, RenderScreen::All);

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
  systems.register_update_system(std::make_unique<SynergyCountingSystem>());
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
  // In headless mode, animations complete instantly, so there are never any
  // blocking animations
  if (render_backend::is_headless_mode) {
    return false;
  }

  for (afterhours::Entity &e :
       EntityQuery().whereHasComponent<IsBlockingAnimationEvent>().gen()) {
    return true;
  }
  return false;
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
