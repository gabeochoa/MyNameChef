#pragma once

#include "../components/animation_event.h"
#include "../components/combat_stats.h"
#include "../components/deferred_flavor_mods.h"
#include "../components/dish_battle_state.h"
#include "../components/dish_level.h"
#include "../components/has_tooltip.h"
#include "../components/is_dish.h"
#include "../components/pairing_clash_modifiers.h"
#include "../components/pending_combat_mods.h"
#include "../components/persistent_combat_modifiers.h"
#include "../components/render_order.h"
#include "../components/side_effect_tracker.h"
#include "../components/transform.h"
#include "../components/trigger_queue.h"
#include "../game_state_manager.h"
#include "../log.h"
#include "../settings.h"
#include <afterhours/ah.h>
#include <map>
#include <string>
#include <unordered_set>

struct AuditSystem : afterhours::System<SideEffectTracker> {
  virtual bool should_run(float) override {
    auto &gsm = GameStateManager::get();
    if (gsm.active_screen != GameStateManager::Screen::Battle) {
      return false;
    }

    auto tracker = afterhours::EntityHelper::get_singleton<SideEffectTracker>();
    if (!tracker.get().has<SideEffectTracker>()) {
      return false;
    }

    const SideEffectTracker &t = tracker.get().get<SideEffectTracker>();
    return t.enabled;
  }

  void for_each_with(afterhours::Entity &, SideEffectTracker &tracker,
                     float) override {
    if (!tracker.enabled) {
      return;
    }

    snapshot_frame(tracker);
  }

private:
  struct ComponentSnapshot {
    bool hasIsDish = false;
    int dishType = 0;
    bool hasDishLevel = false;
    int level = 1;
    bool hasTransform = false;
    float transformX = 0.0f;
    float transformY = 0.0f;
    bool hasHasTooltip = false;
    bool hasHasRenderOrder = false;
  };

  std::map<int, ComponentSnapshot> previous_snapshots;

  void snapshot_frame(SideEffectTracker &tracker) {
    std::unordered_set<int> current_entities;

    for (afterhours::Entity &e :
         afterhours::EntityQuery().whereHasComponent<IsDish>().gen()) {
      current_entities.insert(e.id);

      ComponentSnapshot snap;

      if (e.has<IsDish>()) {
        snap.hasIsDish = true;
        const IsDish &dish = e.get<IsDish>();
        snap.dishType = static_cast<int>(dish.type);
      }

      if (e.has<DishLevel>()) {
        snap.hasDishLevel = true;
        const DishLevel &lvl = e.get<DishLevel>();
        snap.level = lvl.level;
      }

      if (e.has<Transform>()) {
        snap.hasTransform = true;
        const Transform &t = e.get<Transform>();
        snap.transformX = t.pos().x;
        snap.transformY = t.pos().y;
      }

      if (e.has<HasTooltip>()) {
        snap.hasHasTooltip = true;
      }

      if (e.has<HasRenderOrder>()) {
        snap.hasHasRenderOrder = true;
      }

      auto it = previous_snapshots.find(e.id);
      if (it != previous_snapshots.end()) {
        check_violations(tracker, e.id, it->second, snap);
        it->second = snap;
      } else {
        previous_snapshots[e.id] = snap;
      }
    }

    for (auto it = previous_snapshots.begin();
         it != previous_snapshots.end();) {
      if (current_entities.find(it->first) == current_entities.end()) {
        std::string msg = "AUDIT_VIOLATION: Entity " +
                          std::to_string(it->first) + " removed unexpectedly";
        log_error("{}", msg);
        tracker.add_violation(msg);
        it = previous_snapshots.erase(it);
      } else {
        ++it;
      }
    }
  }

  void check_violations(SideEffectTracker &tracker, int entityId,
                        const ComponentSnapshot &prev,
                        const ComponentSnapshot &curr) {
    if (prev.hasIsDish && curr.hasIsDish) {
      if (prev.dishType != curr.dishType) {
        std::string msg = "AUDIT_VIOLATION component=IsDish entity=" +
                          std::to_string(entityId) + " field=dishType before=" +
                          std::to_string(prev.dishType) +
                          " after=" + std::to_string(curr.dishType);
        log_error("{}", msg);
        tracker.add_violation(msg);
      }
    }

    if (prev.hasDishLevel && curr.hasDishLevel) {
      if (prev.level != curr.level) {
        std::string msg = "AUDIT_VIOLATION component=DishLevel entity=" +
                          std::to_string(entityId) +
                          " field=level before=" + std::to_string(prev.level) +
                          " after=" + std::to_string(curr.level);
        log_error("{}", msg);
        tracker.add_violation(msg);
      }
    }

    if (prev.hasTransform && curr.hasTransform) {
      if (prev.transformX != curr.transformX ||
          prev.transformY != curr.transformY) {
        std::string msg = "AUDIT_VIOLATION component=Transform entity=" +
                          std::to_string(entityId) + " field=pos before=(" +
                          std::to_string(prev.transformX) + "," +
                          std::to_string(prev.transformY) + ") after=(" +
                          std::to_string(curr.transformX) + "," +
                          std::to_string(curr.transformY) + ")";
        log_error("{}", msg);
        tracker.add_violation(msg);
      }
    }
  }
};
