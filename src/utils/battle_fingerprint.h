#pragma once

#include "../components/combat_stats.h"
#include "../components/dish_battle_state.h"
#include "../components/dish_level.h"
#include "../components/is_dish.h"
#include "../components/pairing_clash_modifiers.h"
#include "../components/persistent_combat_modifiers.h"
#include "../components/trigger_queue.h"
#include "../query.h"
#include <afterhours/ah.h>
#include <algorithm>
#include <functional>
#include <iomanip>
#include <nlohmann/json.hpp>
#include <sstream>
#include <string>
#include <vector>

struct BattleFingerprint {
  static uint64_t compute() {
    uint64_t hash = 0;

    std::vector<struct DishData> dishes;

    for (afterhours::Entity &e : afterhours::EntityQuery()
                                     .whereHasComponent<IsDish>()
                                     .whereHasComponent<DishBattleState>()
                                     .gen()) {
      DishData data;
      data.entityId = e.id;

      const IsDish &dish = e.get<IsDish>();
      data.dishType = static_cast<int>(dish.type);

      if (e.has<DishLevel>()) {
        const DishLevel &lvl = e.get<DishLevel>();
        data.level = lvl.level;
      }

      const DishBattleState &dbs = e.get<DishBattleState>();
      data.teamSide = static_cast<int>(dbs.team_side);
      data.queueIndex = dbs.queue_index;
      data.phase = static_cast<int>(dbs.phase);
      data.onServeFired = dbs.onserve_fired;

      if (e.has<CombatStats>()) {
        const CombatStats &cs = e.get<CombatStats>();
        data.baseZing = cs.baseZing;
        data.baseBody = cs.baseBody;
        data.currentZing = cs.currentZing;
        data.currentBody = cs.currentBody;
      }

      if (e.has<PairingClashModifiers>()) {
        const PairingClashModifiers &pcm = e.get<PairingClashModifiers>();
        data.pairingZing = pcm.zingDelta;
        data.pairingBody = pcm.bodyDelta;
      }

      if (e.has<PersistentCombatModifiers>()) {
        const PersistentCombatModifiers &pcm =
            e.get<PersistentCombatModifiers>();
        data.persistZing = pcm.zingDelta;
        data.persistBody = pcm.bodyDelta;
      }

      dishes.push_back(data);
    }

    std::sort(dishes.begin(), dishes.end(),
              [](const DishData &a, const DishData &b) {
                if (a.teamSide != b.teamSide)
                  return a.teamSide < b.teamSide;
                if (a.queueIndex != b.queueIndex)
                  return a.queueIndex < b.queueIndex;
                return a.entityId < b.entityId;
              });

    for (const auto &dish : dishes) {
      hash = combine_hash(hash, dish.entityId);
      hash = combine_hash(hash, dish.dishType);
      hash = combine_hash(hash, dish.level);
      hash = combine_hash(hash, dish.teamSide);
      hash = combine_hash(hash, dish.queueIndex);
      hash = combine_hash(hash, dish.phase);
      hash = combine_hash(hash, dish.onServeFired ? 1 : 0);
      hash = combine_hash(hash, dish.baseZing);
      hash = combine_hash(hash, dish.baseBody);
      hash = combine_hash(hash, dish.currentZing);
      hash = combine_hash(hash, dish.currentBody);
      hash = combine_hash(hash, dish.pairingZing);
      hash = combine_hash(hash, dish.pairingBody);
      hash = combine_hash(hash, dish.persistZing);
      hash = combine_hash(hash, dish.persistBody);
    }

    if (auto tq = afterhours::EntityHelper::get_singleton<TriggerQueue>();
        tq.get().has<TriggerQueue>()) {
      const TriggerQueue &queue = tq.get().get<TriggerQueue>();
      hash = combine_hash(hash, static_cast<uint64_t>(queue.events.size()));
    }

    return hash;
  }

  static uint64_t combine_hash(uint64_t hash, uint64_t value) {
    hash ^= value + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    return hash;
  }

private:
  struct DishData {
    int entityId = 0;
    int dishType = 0;
    int level = 1;
    int teamSide = 0;
    int queueIndex = 0;
    int phase = 0;
    bool onServeFired = false;
    int baseZing = 0;
    int baseBody = 0;
    int currentZing = 0;
    int currentBody = 0;
    int pairingZing = 0;
    int pairingBody = 0;
    int persistZing = 0;
    int persistBody = 0;
  };
};

constexpr const char *GAME_STATE_CLIENT_VERSION = "0.1.0";

inline std::string compute_game_state_checksum(const nlohmann::json &state) {
  std::string json_str = state.dump();
  uint64_t hash = 0;
  for (char c : json_str) {
    hash = BattleFingerprint::combine_hash(hash, static_cast<uint64_t>(c));
  }
  std::stringstream ss;
  ss << std::hex << std::setfill('0') << std::setw(16) << hash;
  return ss.str();
}
