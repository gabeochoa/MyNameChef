#pragma once

#include <afterhours/ah.h>
#include <optional>
#include <string>

namespace server {
struct ServerContext {
  afterhours::SystemManager systems;
  afterhours::Entity *manager_entity;

  ServerContext() : manager_entity(nullptr) {}

  static ServerContext initialize();
  void initialize_singletons();
  void register_battle_systems();
  void register_server_systems();
  bool is_battle_complete() const;
};
} // namespace server
