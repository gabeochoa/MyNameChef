#pragma once

#include "../components/network_info.h"
#include "../log.h"
#include <afterhours/ah.h>
#include <cstdlib>
#include <httplib.h>

struct NetworkSystem : afterhours::System<NetworkInfo> {
  static constexpr const char *SERVER_IP = "localhost";
  static constexpr int SERVER_PORT = 8080;

  // Read check interval from environment variable (default 10.0s)
  // Tests can set NETWORK_CHECK_INTERVAL_SECONDS to speed up detection
  static float get_check_interval() {
    const char *env = std::getenv("NETWORK_CHECK_INTERVAL_SECONDS");
    if (env) {
      return std::atof(env);
    }
    return 10.0f;
  }

  // Read HTTP timeouts from environment variable (default 2000ms read, 1000ms
  // connect) Tests can set NETWORK_TIMEOUT_MS to speed up failure detection
  static int get_read_timeout_ms() {
    const char *env = std::getenv("NETWORK_TIMEOUT_MS");
    if (env) {
      return std::atoi(env);
    }
    return 2000;
  }

  static int get_connect_timeout_ms() {
    const char *env = std::getenv("NETWORK_TIMEOUT_MS");
    if (env) {
      return std::atoi(env);
    }
    return 1000;
  }

  void for_each_with(afterhours::Entity &, NetworkInfo &networkInfo,
                     float dt) override {
    float check_interval = get_check_interval();

    networkInfo.timeSinceLastCheck -= dt;
    if (networkInfo.timeSinceLastCheck > 0.0f) {
      return;
    }

    // Reset countdown after check
    networkInfo.timeSinceLastCheck = check_interval;

    ServerAddress addr;
    addr.ip = SERVER_IP;
    addr.port = SERVER_PORT;
    networkInfo.serverAddress = addr;

    bool connected = check_server_health(addr);
    networkInfo.hasConnection = connected;

    // Track if we've ever had a successful connection
    if (connected) {
      log_info("NETWORK: Server connection OK");
    } else {
      log_warn("NETWORK: Server connection failed");
    }
  }

  // Public method for initial connection check during setup
  static bool check_server_health(const ServerAddress &addr) {
    httplib::Client client(addr.ip, addr.port);
    int read_timeout_ms = get_read_timeout_ms();
    int connect_timeout_ms = get_connect_timeout_ms();
    client.set_read_timeout(read_timeout_ms / 1000,
                            (read_timeout_ms % 1000) * 1000);
    client.set_connection_timeout(connect_timeout_ms / 1000,
                                  (connect_timeout_ms % 1000) * 1000);

    auto res = client.Get("/health");
    bool connected = res && res->status == 200;

    if (!connected) {
      log_info("NETWORK: Health check failed - res: {}, status: {}",
               res != nullptr, res ? res->status : -1);
    }

    return connected;
  }
};
