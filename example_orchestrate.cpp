// clang++ -Wall -Wextra -std=c++11 -I. orchestrate.cpp example_orchestrate.cpp
// -lcurl
#include "orchestrate.hpp"

#include <iostream>

using namespace mk::wac;

int main() {
  std::vector<std::string> urls;
  OrchestrateClient client;
  if (client.get_urls("IT", {"HUMR", "ENV"}, 100, &urls) != true) {
    std::clog << "Failed to get_urls" << std::endl;
    return 1;
  }
  for (auto url : urls) {
    std::clog << url << std::endl;
  }
  return 0;
}
