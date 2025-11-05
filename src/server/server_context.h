#pragma once

#include <afterhours/ah.h>
#include <optional>
#include <string>

namespace server {
  struct ServerContext {
    afterhours::SystemManager systems;
    afterhours::Entity manager_entity;
    
    static ServerContext initialize();
    void initialize_singletons();
    void register_battle_systems();
    bool is_battle_complete() const;
  };
}

