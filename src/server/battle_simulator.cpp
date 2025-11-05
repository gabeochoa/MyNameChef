#include "battle_simulator.h"
#include "file_storage.h"
#include "../components/battle_load_request.h"
#include "../seeded_rng.h"
#include <afterhours/ah.h>

namespace server {
BattleSimulator::BattleSimulator()
    : ctx(ServerContext::initialize()), seed(0), battle_active(false),
      simulation_time(0.0f) {}

void BattleSimulator::start_battle(const nlohmann::json &player_team_json,
                                   const nlohmann::json &opponent_team_json,
                                   uint64_t battle_seed) {

  seed = battle_seed;
  simulation_time = 0.0f;
  battle_active = true;

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
} // namespace server
