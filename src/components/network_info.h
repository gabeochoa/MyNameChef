#pragma once

#include <afterhours/ah.h>
#include <optional>
#include <string>

struct ServerAddress {
  std::string ip;
  int port = 8080;
};

using OptServerAddr = std::optional<ServerAddress>;

struct NetworkInfo : afterhours::BaseComponent {
  bool hasConnection = false;
  float timeSinceLastCheck = 0.0f;
  OptServerAddr serverAddress = std::nullopt;

  NetworkInfo() = default;

  static bool has_connection() {
    auto networkInfoOpt =
        afterhours::EntityHelper::get_singleton<NetworkInfo>();
    afterhours::Entity &entity = networkInfoOpt.get();
    if (!entity.has<NetworkInfo>()) {
      return false;
    }
    return entity.get<NetworkInfo>().hasConnection;
  }

  static bool is_disconnected() { return !has_connection(); }

  static OptServerAddr get_server_address() {
    auto networkInfoOpt =
        afterhours::EntityHelper::get_singleton<NetworkInfo>();
    afterhours::Entity &entity = networkInfoOpt.get();
    if (!entity.has<NetworkInfo>()) {
      return std::nullopt;
    }
    return entity.get<NetworkInfo>().serverAddress;
  }
};
