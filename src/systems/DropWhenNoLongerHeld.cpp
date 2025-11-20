#include "DropWhenNoLongerHeld.h"
#include "../components/can_drop_onto.h"
#include "../components/dish_level.h"
#include "../components/is_dish.h"
#include "../components/is_held.h"
#include "../components/is_inventory_item.h"
#include "../components/is_shop_item.h"
#include "../dish_types.h"
#include "../game_state_manager.h"
#include "../log.h"
#include "../query.h"
#include "../shop.h"
#include "../testing/test_input.h"

bool DropWhenNoLongerHeld::should_run(float) {
  auto &gsm = GameStateManager::get();
  bool should = gsm.active_screen == GameStateManager::Screen::Shop;
  if (should) {
    // Count entities with IsHeld to see if system should process anything
    int held_count = 0;
    for (auto &ref : EQ({.force_merge = true}).whereHasComponent<IsHeld>().gen()) {
      (void)ref; // Suppress unused variable warning
      held_count++;
    }
    log_error("DROP_WHEN_NO_LONGER_HELD: should_run=true, found {} entities with IsHeld", held_count);
  }
  return should;
}

bool DropWhenNoLongerHeld::can_drop_item(const Entity &entity,
                                         const IsDropSlot &drop_slot) {
  return (entity.has<IsInventoryItem>() && drop_slot.accepts_inventory_items) ||
         (entity.has<IsShopItem>() && drop_slot.accepts_shop_items);
}

int DropWhenNoLongerHeld::get_slot_id(const Entity &entity) {
  if (entity.has<IsInventoryItem>()) {
    return entity.get<IsInventoryItem>().slot;
  } else if (entity.has<IsShopItem>()) {
    return entity.get<IsShopItem>().slot;
  }
  return -1;
}

void DropWhenNoLongerHeld::set_slot_id(Entity &entity, int slot_id) {
  if (entity.has<IsInventoryItem>()) {
    entity.get<IsInventoryItem>().slot = slot_id;
  } else if (entity.has<IsShopItem>()) {
    entity.get<IsShopItem>().slot = slot_id;
  }
}

afterhours::OptEntity
DropWhenNoLongerHeld::find_item_in_slot(int slot_id,
                                        const Entity &exclude_entity) {
  // First check inventory items - need to check the slot field, not use
  // whereSlotID (which checks IsDropSlot entities, not items)
  for (auto &ref : EQ().whereHasComponent<IsInventoryItem>()
                       .whereNotID(exclude_entity.id)
                       .gen()) {
    const auto &inv = ref.get().get<IsInventoryItem>();
    if (inv.slot == slot_id) {
      return afterhours::OptEntity(ref.get());
    }
  }

  // Then check shop items
  for (auto &ref : EQ().whereHasComponent<IsShopItem>()
                       .whereNotID(exclude_entity.id)
                       .gen()) {
    const auto &shop = ref.get().get<IsShopItem>();
    if (shop.slot == slot_id) {
      return afterhours::OptEntity(ref.get());
    }
  }

  return afterhours::OptEntity();
}

void DropWhenNoLongerHeld::snap_back_to_original(Entity &entity,
                                                 const IsHeld &held) {
  entity.get<Transform>().position = held.original_position;
  entity.removeComponent<IsHeld>();
}

bool DropWhenNoLongerHeld::can_merge_dishes(const Entity &entity,
                                            const Entity &target_item) {
  if (!entity.has<IsDish>() || !target_item.has<IsDish>()) {
    return false;
  }

  auto &entity_dish = entity.get<IsDish>();
  auto &target_dish = target_item.get<IsDish>();

  // Can only merge same dish types
  if (entity_dish.type != target_dish.type) {
    return false;
  }

  // Both must have DishLevel components
  if (!entity.has<DishLevel>() || !target_item.has<DishLevel>()) {
    return false;
  }

  auto &entity_level = entity.get<DishLevel>();
  auto &target_level = target_item.get<DishLevel>();

  // Allow merge if donor level <= target level
  return entity_level.level <= target_level.level;
}

void DropWhenNoLongerHeld::merge_dishes(Entity &entity, Entity *target_item,
                                        Entity *, const Transform &,
                                        const IsHeld &held) {
  // If the dropped item came from the shop and we're merging into an
  // inventory item, charge before consuming the dropped item
  if (entity.has<IsShopItem>() && target_item &&
      target_item->has<IsInventoryItem>()) {
    auto &dish = entity.get<IsDish>();
    if (!charge_for_shop_purchase(dish.type)) {
      snap_back_to_original(entity, held);
      return;
    }
    entity.removeComponentIfExists<Freezeable>();
  }

  auto &target_level = target_item->get<DishLevel>();
  auto &donor_level = entity.get<DishLevel>();

  // Add donor's full contribution to target
  target_level.add_merge_value(donor_level.contribution_value());

  // Don't move the target item - it should stay where it is
  // The target item keeps its current position (inventory or shop)

  // Free the original slot if it was occupied (do this before cleanup)
  int original_slot_id = get_slot_id(entity);
  if (original_slot_id >= 0) {
    if (auto original_slot = EQ().whereHasComponent<IsDropSlot>()
                                 .whereSlotID(original_slot_id)
                                 .gen_first()) {
      original_slot->get<IsDropSlot>().occupied = false;
    }
  }

  // Remove the dropped item - mark for cleanup
  // Don't call cleanup immediately as it may interfere with the current frame
  entity.removeComponent<IsHeld>();
  entity.cleanup = true;
}

bool DropWhenNoLongerHeld::try_purchase_shop_item(Entity &entity,
                                                  Entity *drop_slot) {
  bool has_shop_item = entity.has<IsShopItem>();
  bool accepts_inventory = drop_slot->get<IsDropSlot>().accepts_inventory_items;
  
  if (!has_shop_item || !accepts_inventory) {
    log_error("TRY_PURCHASE: Early return - entity.has<IsShopItem>()={}, accepts_inventory_items={}", 
             has_shop_item, accepts_inventory);
    // If entity doesn't have IsShopItem, we can't purchase it, but we still return true
    // to allow the drop to proceed (it's not a shop purchase)
    // BUT - if it doesn't have IsShopItem, we shouldn't remove it!
    // The issue might be that set_slot_id is being called after this, which might
    // be modifying the entity in a way that removes IsShopItem
    return true;
  }

  auto &dish = entity.get<IsDish>();
  log_error("TRY_PURCHASE: Calling charge_for_shop_purchase for dish type {}", static_cast<int>(dish.type));
  if (!charge_for_shop_purchase(dish.type)) {
    const int price = get_dish_info(dish.type).price;
    make_toast("Not enough gold! Need " + std::to_string(price) + " gold");
    log_error("TRY_PURCHASE: charge_for_shop_purchase returned false");
    return false;
  }

  log_error("TRY_PURCHASE: charge_for_shop_purchase succeeded, removing IsShopItem and adding IsInventoryItem");
  entity.removeComponent<IsShopItem>();
  entity.addComponent<IsInventoryItem>();
  entity.get<IsInventoryItem>().slot = drop_slot->get<IsDropSlot>().slot_id;
  entity.removeComponentIfExists<Freezeable>();

  return true;
}

void DropWhenNoLongerHeld::drop_into_empty_slot(Entity &entity,
                                                Entity *drop_slot,
                                                const Transform &transform,
                                                const IsHeld &held) {
  vec2 slot_center = drop_slot->get<Transform>().center();
  entity.get<Transform>().position = slot_center - transform.size * 0.5f;

  // Handle selling: dropping an inventory item onto SELL_SLOT_ID sells it
  if (drop_slot->get<IsDropSlot>().slot_id == SELL_SLOT_ID &&
      entity.has<IsInventoryItem>()) {
    // Free the original inventory slot
    int original_slot_id = get_slot_id(entity);
    if (original_slot_id >= 0) {
      if (auto original_slot = EQ().whereHasComponent<IsDropSlot>()
                                   .whereSlotID(original_slot_id)
                                   .gen_first()) {
        original_slot->get<IsDropSlot>().occupied = false;
      }
    }

    auto wallet_entity = EntityHelper::get_singleton<Wallet>();
    if (wallet_entity.get().has<Wallet>()) {
      auto &wallet = wallet_entity.get().get<Wallet>();
      wallet.gold += 1; // flat refund for now
    }
    entity.cleanup = true; // remove the sold item
    return;
  }

  bool purchase_result = try_purchase_shop_item(entity, drop_slot);
  log_error("DROP_INTO_EMPTY_SLOT: try_purchase_shop_item returned {}, entity.has<IsShopItem>()={}, entity.has<IsInventoryItem>()={}", 
           purchase_result, entity.has<IsShopItem>(), entity.has<IsInventoryItem>());
  
  if (!purchase_result) {
    snap_back_to_original(entity, held);
    return;
  }

  // Only set slot_id if the entity has IsInventoryItem (purchase succeeded)
  // or IsShopItem (early return, not a purchase)
  if (entity.has<IsInventoryItem>() || entity.has<IsShopItem>()) {
    set_slot_id(entity, drop_slot->get<IsDropSlot>().slot_id);
  }
  drop_slot->get<IsDropSlot>().occupied = true;
}

bool DropWhenNoLongerHeld::swap_items(Entity &entity, Entity *occupied_slot,
                                      const Transform &transform,
                                      const IsHeld &held) {
  int slot_id = occupied_slot->get<IsDropSlot>().slot_id;

  afterhours::OptEntity item_in_slot = find_item_in_slot(slot_id, entity);
  if (!item_in_slot.has_value()) {
    return false;
  }

  Entity &item = *item_in_slot.value();

  // Check if we can merge instead of swap
  if (can_merge_dishes(entity, item)) {
    merge_dishes(entity, &item, occupied_slot, transform, held);
    return true;
  }

  // Only prevent swapping if one is shop item and one is inventory item
  // Allow shop-to-shop and inventory-to-inventory swaps
  if ((entity.has<IsShopItem>() && item.has<IsInventoryItem>()) ||
      (entity.has<IsInventoryItem>() && item.has<IsShopItem>())) {
    snap_back_to_original(entity, held);
    return false;
  }

  int original_slot_id = get_slot_id(entity);
  int target_slot_id = occupied_slot->get<IsDropSlot>().slot_id;

  bool entity_was_frozen = false;
  if (entity.has<Freezeable>()) {
    entity_was_frozen = entity.get<Freezeable>().isFrozen;
  }
  bool item_was_frozen = false;
  if (item.has<Freezeable>()) {
    item_was_frozen = item.get<Freezeable>().isFrozen;
  }

  vec2 slot_center = occupied_slot->get<Transform>().center();
  entity.get<Transform>().position = slot_center - transform.size * 0.5f;
  item.get<Transform>().position = held.original_position;

  set_slot_id(entity, target_slot_id);
  set_slot_id(item, original_slot_id);

  if (entity.has<Freezeable>()) {
    entity.get<Freezeable>().isFrozen = entity_was_frozen;
  }
  if (item.has<Freezeable>()) {
    item.get<Freezeable>().isFrozen = item_was_frozen;
  }

  occupied_slot->get<IsDropSlot>().occupied = true;
  if (original_slot_id >= 0) {
    auto original_slot = EQ().whereHasComponent<IsDropSlot>()
                             .whereSlotID(original_slot_id)
                             .gen_first();
    if (original_slot) {
      original_slot->get<IsDropSlot>().occupied = true;
    }
  }
  return false;
}

void DropWhenNoLongerHeld::for_each_with(Entity &entity, IsHeld &held,
                                         Transform &transform, float) {
  log_error("DROP_WHEN_NO_LONGER_HELD: System running for entity {}, has IsShopItem={}, has IsInventoryItem={}", 
           entity.id, entity.has<IsShopItem>(), entity.has<IsInventoryItem>());
  
  // Check test input wrapper first, then fall back to real input
  // If simulation is active, use test_input; otherwise use real input
  bool button_released = test_input::is_simulation_active()
                             ? test_input::is_mouse_button_released(raylib::MOUSE_BUTTON_LEFT)
                             : afterhours::input::is_mouse_button_released(raylib::MOUSE_BUTTON_LEFT);
  
  log_error("DROP_WHEN_NO_LONGER_HELD: button_released={}, simulation_active={}", 
           button_released, test_input::is_simulation_active());
  
  if (!button_released) {
    vec2 mouse_pos = test_input::is_simulation_active()
                         ? test_input::get_mouse_position()
                         : afterhours::input::get_mouse_position();
    entity.get<Transform>().position =
        mouse_pos + held.offset - transform.size * 0.5f;
    return;
  }

  vec2 mouse_pos = test_input::is_simulation_active()
                       ? test_input::get_mouse_position()
                       : afterhours::input::get_mouse_position();
  Rectangle mouse_rect = {mouse_pos.x, mouse_pos.y, 1, 1};
  
  log_error("DROP_WHEN_NO_LONGER_HELD: Mouse released, searching for drop slots at ({}, {})", 
           mouse_pos.x, mouse_pos.y);

  Entity *best_drop_slot = nullptr;
  Entity *occupied_slot = nullptr;
  float best_distance = std::numeric_limits<float>::max();

  int drop_slot_count = 0;
  for (auto &ref : EQ().whereHasComponent<Transform>()
                       .whereHasComponent<CanDropOnto>()
                       .whereLambda([](const Entity &e) {
                         return e.get<CanDropOnto>().enabled;
                       })
                       .whereHasComponent<IsDropSlot>()
                       .whereLambda([&](const Entity &e) {
                         return can_drop_item(entity, e.get<IsDropSlot>());
                       })
                       .whereOverlaps(mouse_rect)
                       .gen()) {
    drop_slot_count++;
    auto &slot_entity = ref.get();

    vec2 slot_center = slot_entity.get<Transform>().center();
    float distance =
        Vector2Distance(raylib::Vector2{mouse_pos.x, mouse_pos.y},
                        raylib::Vector2{slot_center.x, slot_center.y});

    log_error("DROP_WHEN_NO_LONGER_HELD: Found drop slot {} at ({}, {}), distance={}, occupied={}, accepts_inventory={}, accepts_shop={}", 
             slot_entity.id, slot_center.x, slot_center.y, distance, 
             slot_entity.get<IsDropSlot>().occupied,
             slot_entity.get<IsDropSlot>().accepts_inventory_items,
             slot_entity.get<IsDropSlot>().accepts_shop_items);

    if (slot_entity.get<IsDropSlot>().occupied) {
      occupied_slot = &slot_entity;
    } else if (distance < best_distance) {
      best_distance = distance;
      best_drop_slot = &slot_entity;
    }
  }
  
  log_error("DROP_WHEN_NO_LONGER_HELD: Found {} drop slots, best_drop_slot={}, occupied_slot={}", 
           drop_slot_count, best_drop_slot != nullptr, occupied_slot != nullptr);

  bool was_merged = false;
  if (best_drop_slot) {
    log_error("DROP_WHEN_NO_LONGER_HELD: Calling drop_into_empty_slot");
    drop_into_empty_slot(entity, best_drop_slot, transform, held);
  } else if (occupied_slot) {
    log_error("DROP_WHEN_NO_LONGER_HELD: Calling swap_items");
    was_merged = swap_items(entity, occupied_slot, transform, held);
  } else {
    log_error("DROP_WHEN_NO_LONGER_HELD: No drop slot found, snapping back");
    snap_back_to_original(entity, held);
  }

  if (!was_merged) {
    entity.removeComponentIfExists<IsHeld>();
  }
}
