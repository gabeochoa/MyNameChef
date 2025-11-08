#pragma once

#include <string>

namespace http_helpers {

struct ServerUrlParts {
  std::string host;
  int port;
  bool success;
};

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

  try {
    size_t protocol_end = 0;

    // Find protocol (http:// or https://)
    size_t protocol_pos = server_url.find("://");
    if (protocol_pos != std::string::npos) {
      protocol_end = protocol_pos + 3;
    }

    // Find port separator and path separator
    size_t colon_pos = server_url.find(":", protocol_end);
    size_t slash_pos = server_url.find("/", protocol_end);

    if (colon_pos != std::string::npos &&
        (slash_pos == std::string::npos || colon_pos < slash_pos)) {
      // Port is specified
      parts.host = server_url.substr(protocol_end, colon_pos - protocol_end);
      size_t port_end =
          (slash_pos != std::string::npos) ? slash_pos : server_url.length();
      try {
        parts.port = std::stoi(
            server_url.substr(colon_pos + 1, port_end - colon_pos - 1));
      } catch (...) {
        return parts; // Failed to parse port
      }
    } else {
      // No port specified, use default
      size_t end =
          (slash_pos != std::string::npos) ? slash_pos : server_url.length();
      parts.host = server_url.substr(protocol_end, end - protocol_end);
    }

    parts.success = true;
  } catch (...) {
    // Parsing failed
    return parts;
  }

  return parts;
}

} // namespace http_helpers

