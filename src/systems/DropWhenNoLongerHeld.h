#pragma once

#include <afterhours/ah.h>
//

#include "../components/is_drop_slot.h"
#include "../components/is_held.h"
#include "../components/transform.h"

struct DropWhenNoLongerHeld : afterhours::System<IsHeld, Transform> {
  virtual bool should_run(float) override;

private:
  bool can_drop_item(const afterhours::Entity &entity,
                     const IsDropSlot &drop_slot);
  int get_slot_id(const afterhours::Entity &entity);
  void set_slot_id(afterhours::Entity &entity, int slot_id);
  afterhours::OptEntity
  find_item_in_slot(int slot_id, const afterhours::Entity &exclude_entity);
  void snap_back_to_original(afterhours::Entity &entity, const IsHeld &held);
  bool can_merge_dishes(const afterhours::Entity &entity,
                        const afterhours::Entity &target_item);
  void merge_dishes(afterhours::Entity &entity, afterhours::Entity *target_item,
                    afterhours::Entity *, const Transform &,
                    const IsHeld &held);
  bool try_purchase_shop_item(afterhours::Entity &entity,
                              afterhours::Entity *drop_slot);
  void drop_into_empty_slot(afterhours::Entity &entity,
                            afterhours::Entity *drop_slot,
                            const Transform &transform, const IsHeld &held);
  void swap_items(afterhours::Entity &entity, afterhours::Entity *occupied_slot,
                  const Transform &transform, const IsHeld &held);

public:
  void for_each_with(afterhours::Entity &entity, IsHeld &held,
                     Transform &transform, float) override;
};