#include "battle_simulator.h"
#include "../components/battle_load_request.h"
#include "../components/combat_queue.h"
#include "../components/trigger_queue.h"
#include "../seeded_rng.h"
#include "file_storage.h"
#include <afterhours/ah.h>

namespace server {
BattleSimulator::BattleSimulator()
    : ctx(ServerContext::initialize()), seed(0), battle_active(false),
      simulation_time(0.0f), accumulated_events() {}

void BattleSimulator::start_battle(const nlohmann::json &player_team_json,
                                   const nlohmann::json &opponent_team_json,
                                   uint64_t battle_seed) {

  seed = battle_seed;
  simulation_time = 0.0f;
  battle_active = true;
  accumulated_events.clear();

  SeededRng::get().set_seed(seed);

  FileStorage::ensure_directory_exists("output/battles");

  std::string player_temp =
      "output/battles/temp_player_" + std::to_string(seed) + ".json";
  std::string opponent_temp =
      "output/battles/temp_opponent_" + std::to_string(seed) + ".json";

  FileStorage::save_json_to_file(player_temp, player_team_json);
  FileStorage::save_json_to_file(opponent_temp, opponent_team_json);

  afterhours::Entity &request_entity = afterhours::EntityHelper::createEntity();
  request_entity.addComponent<BattleLoadRequest>();
  BattleLoadRequest &req = request_entity.get<BattleLoadRequest>();
  req.playerJsonPath = player_temp;
  req.opponentJsonPath = opponent_temp;
  req.loaded = false;

  afterhours::EntityHelper::registerSingleton<BattleLoadRequest>(
      request_entity);

  afterhours::EntityHelper::merge_entity_arrays();
}

void BattleSimulator::update(float dt) {
  if (!battle_active || is_complete()) {
    return;
  }

  int course_index = 0;
  auto cq_entity = afterhours::EntityHelper::get_singleton<CombatQueue>();
  if (cq_entity.get().has<CombatQueue>()) {
    const CombatQueue &cq = cq_entity.get().get<CombatQueue>();
    course_index = cq.current_index;
  }

  track_events(simulation_time, course_index);

  simulation_time += dt;

  const float fixed_dt = 1.0f / 60.0f;
  ctx.systems.run(fixed_dt);
}

bool BattleSimulator::is_complete() const { return ctx.is_battle_complete(); }

nlohmann::json BattleSimulator::get_battle_state() const {
  return nlohmann::json{{"seed", seed},
                        {"complete", is_complete()},
                        {"simulation_time", simulation_time}};
}

void BattleSimulator::track_events(float timestamp, int course_index) {
  auto tq_entity = afterhours::EntityHelper::get_singleton<TriggerQueue>();
  if (!tq_entity.get().has<TriggerQueue>()) {
    return;
  }

  const TriggerQueue &queue = tq_entity.get().get<TriggerQueue>();
  for (const TriggerEvent &ev : queue.events) {
    BattleEvent battle_event;
    battle_event.hook = ev.hook;
    battle_event.sourceEntityId = ev.sourceEntityId;
    battle_event.slotIndex = ev.slotIndex;
    battle_event.teamSide = ev.teamSide;
    battle_event.timestamp = timestamp;
    battle_event.courseIndex = course_index;
    battle_event.payloadInt = ev.payloadInt;
    battle_event.payloadFloat = ev.payloadFloat;
    accumulated_events.push_back(battle_event);
  }
}
} // namespace server
