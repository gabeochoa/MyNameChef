#pragma once

#include "../components/network_info.h"
#include "../log.h"
#include <afterhours/ah.h>
#include <httplib.h>

struct NetworkSystem : afterhours::System<NetworkInfo> {
  static constexpr float CHECK_INTERVAL = 10.0f;
  static constexpr const char *SERVER_IP = "localhost";
  static constexpr int SERVER_PORT = 8080;

  void for_each_with(afterhours::Entity &, NetworkInfo &networkInfo,
                     float dt) override {
    networkInfo.timeSinceLastCheck += dt;

    bool should_check = networkInfo.timeSinceLastCheck >= CHECK_INTERVAL;
    
    if (!should_check && networkInfo.serverAddress.has_value()) {
      return;
    }

    if (should_check) {
      networkInfo.timeSinceLastCheck = 0.0f;
    }

    ServerAddress addr;
    addr.ip = SERVER_IP;
    addr.port = SERVER_PORT;
    networkInfo.serverAddress = addr;

    bool connected = check_server_health(addr);
    networkInfo.hasConnection = connected;

    if (connected) {
      log_info("NETWORK: Server connection OK");
    } else {
      log_warn("NETWORK: Server connection failed");
    }
  }

private:
  static bool check_server_health(const ServerAddress &addr) {
    httplib::Client client(addr.ip, addr.port);
    client.set_read_timeout(2, 0);
    client.set_connection_timeout(1, 0);

    auto res = client.Get("/health");
    return res && res->status == 200;
  }
};
