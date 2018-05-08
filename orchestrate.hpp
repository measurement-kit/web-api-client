// Part of Measurement Kit <https://measurement-kit.github.io/>.
// Measurement Kit is free software under the BSD license. See AUTHORS
// and LICENSE for more information on the copying conditions.
#ifndef MEASUREMENT_KIT_WEB_API_CLIENT_ORCHESTRATE_HPP
#define MEASUREMENT_KIT_WEB_API_CLIENT_ORCHESTRATE_HPP

#include <stdint.h>

#include <memory>
#include <string>
#include <vector>

namespace mk {
namespace wac {

enum class BackendType { ONION, HTTPS, DOMAIN_FRONTED };

class OrchestrateSettings {
 public:
  std::string address;
  BackendType type;
  std::string front;
};

class OrchestrateClient {
 public:
  OrchestrateSettings settings;

  bool get_urls(const std::string &country_code,
                const std::vector<std::string> &category_codes, size_t limit,
                std::vector<std::string> *urls) noexcept;
};

}  // namespace wac
}  // namespace mk
#endif
