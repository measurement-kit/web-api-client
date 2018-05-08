#pragma once

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
                const std::vector<std::string> &category_codes, const int limit,
                std::vector<std::string> *urls) noexcept;
};

}  // namespace wac
}  // namespace mk
