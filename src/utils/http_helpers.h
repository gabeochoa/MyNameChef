#pragma once

#include <cstdlib>
#include <string>

namespace http_helpers {

struct ServerUrlParts {
  std::string host;
  int port;
  bool success;
  bool is_https = false; // issue 28: scheme retained, never silently downgraded
};

// Get server URL from environment variable or use default
inline std::string get_server_url() {
  const char *env = std::getenv("INTEGRATION_SERVER_URL");
  return env ? std::string(env) : std::string("http://localhost:8080");
}

// Parse server URL into host and port
// Handles URLs like:
//   - "http://localhost:8080" -> host="localhost", port=8080
//   - "https://example.com:443/path" -> host="example.com", port=443
//   - "localhost:8080" -> host="localhost", port=8080
//   - "localhost" -> host="localhost", port=8080 (default)
// Returns ServerUrlParts with success=false if parsing fails
inline ServerUrlParts parse_server_url(const std::string &server_url,
                                       int default_port = 8080) {
  ServerUrlParts parts;
  parts.port = default_port;
  parts.success = false;

  if (server_url.empty()) {
    return parts;
  }

  // Issues 28/29: validate scheme, host, full-numeric port 1-65535, [IPv6].
  size_t protocol_end = 0;
  size_t protocol_pos = server_url.find("://");
  if (protocol_pos != std::string::npos) {
    std::string scheme = server_url.substr(0, protocol_pos);
    if (scheme == "https") { parts.is_https = true; parts.port = 443; }
    else if (scheme == "http") { parts.port = 80; }
    else return parts;
    protocol_end = protocol_pos + 3;
  }
  size_t slash_pos = server_url.find("/", protocol_end);
  size_t auth_end = (slash_pos == std::string::npos) ? server_url.size() : slash_pos;
  std::string authority = server_url.substr(protocol_end, auth_end - protocol_end);
  if (authority.empty()) return parts;
  std::string port_str;
  if (!authority.empty() && authority[0] == '[') {
    size_t close = authority.find(']');
    if (close == std::string::npos) return parts;
    parts.host = authority.substr(1, close - 1);
    if (close + 1 < authority.size()) {
      if (authority[close + 1] != ':') return parts;
      port_str = authority.substr(close + 2);
    }
  } else {
    size_t colon = authority.rfind(':');
    if (colon != std::string::npos) { parts.host = authority.substr(0, colon); port_str = authority.substr(colon + 1); }
    else parts.host = authority;
  }
  if (parts.host.empty()) return parts;
  if (!port_str.empty()) {
    if (port_str.find_first_not_of("0123456789") != std::string::npos) return parts;
    long p = std::strtol(port_str.c_str(), nullptr, 10);
    if (p < 1 || p > 65535) return parts;
    parts.port = static_cast<int>(p);
  }
  parts.success = true;
  return parts;
}

} // namespace http_helpers
