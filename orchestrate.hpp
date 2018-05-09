// Part of Measurement Kit <https://measurement-kit.github.io/>.
// Measurement Kit is free software under the BSD license. See AUTHORS
// and LICENSE for more information on the copying conditions.
#ifndef MEASUREMENT_KIT_WEB_API_CLIENT_ORCHESTRATE_HPP
#define MEASUREMENT_KIT_WEB_API_CLIENT_ORCHESTRATE_HPP

#include <stdint.h>

#include <memory>
#include <string>
#include <vector>

#include <curl/curl.h>

namespace mk {
namespace wac {

enum class BackendType { ONION, HTTP, HTTPS, DOMAIN_FRONTED };

class OrchestrateSettings {
 public:
  std::string address;
  BackendType type = BackendType::HTTPS;
  std::string front;
  std::string socks_config;
};

class OrchestrateClient {
 public:
  OrchestrateSettings settings;

  bool get_urls(const std::string &country_code,
                const std::vector<std::string> &category_codes, size_t limit,
                std::vector<std::string> *urls) noexcept;

  /* overridable for testing */
  virtual CURL *curl_easy_init() noexcept;
  /*
  curl_easy_setopt
  curl_slist_append
  curl_easy_perform
  */
};

}  // namespace wac
}  // namespace mk
#endif
