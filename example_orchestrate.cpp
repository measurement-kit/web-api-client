// Part of Measurement Kit <https://measurement-kit.github.io/>.
// Measurement Kit is free software under the BSD license. See AUTHORS
// and LICENSE for more information on the copying conditions.

#include "orchestrate.hpp"

#include <iostream>

using namespace mk::wac;

int main() {
  std::vector<std::string> urls;
  OrchestrateClient client;
  /*
  client.settings.address = "d36jct0wniod5z.cloudfront.net";
  client.settings.front = "a0.awsstatic.com";
  client.settings.type = BackendType::DOMAIN_FRONTED;
  */
  client.settings.address = "65as4puv7ecde5q2.onion";
  client.settings.type = BackendType::ONION;
  client.settings.socks_config = "socks5h://127.0.0.1:9050";
  if (!client.get_urls("IT", {"HUMR", "ENV"}, 10, &urls)) {
    return 1;  // error already printed
  }
  for (auto url : urls) {
    std::clog << url << std::endl;
  }
  return 0;
}
