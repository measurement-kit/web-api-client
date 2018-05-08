// Part of Measurement Kit <https://measurement-kit.github.io/>.
// Measurement Kit is free software under the BSD license. See AUTHORS
// and LICENSE for more information on the copying conditions.

#include "orchestrate.hpp"

#include <iostream>

using namespace mk::wac;

int main() {
  std::vector<std::string> urls;
  OrchestrateClient client;
  if (!client.get_urls("IT", {"HUMR", "ENV"}, 100, &urls)) {
    return 1; // error already printed
  }
  for (auto url : urls) {
    std::clog << url << std::endl;
  }
  return 0;
}
