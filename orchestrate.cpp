#include "orchestrate.hpp"

#include <iostream>
#include <sstream>

#include <curl/curl.h>
#include <nlohmann/json.hpp>

static size_t body_cb(char *ptr, size_t size, size_t nmemb, void *userdata) {
  if (nmemb <= 0) {
    return 0;  // This means "no body"
  }
  if (size > SIZE_MAX / nmemb) {
    std::clog << "fatal: unexpected sizes in cURL callback" << std::endl;
    return 0;
  }
  auto realsiz = size * nmemb;  // Overflow not possible (see above)
  auto ss = static_cast<std::stringstream *>(userdata);
  (*ss) << std::string{ptr, realsiz};
  return nmemb;
}

namespace mk {
namespace wac {

bool OrchestrateClient::get_urls(const std::string &country_code,
                                 const std::vector<std::string> &category_codes,
                                 const int limit,
                                 std::vector<std::string> *urls) noexcept {
  std::stringstream response_body;
  CURL *curl = curl_easy_init();
  if (curl == nullptr) {
    std::clog << "fatal: curl_easy_init() failed" << std::endl;
    return false;
  }
  std::clog << "cURL initialized" << std::endl;

  // XXX compose URL with the arguments
  // country_code, category_codes
  constexpr auto url = "https://events.proteus.test.ooni.io/api/v1/urls";

  if (curl_easy_setopt(curl, CURLOPT_URL, url) != CURLE_OK) {
    std::clog << "fatal: curl_easy_setopt(CURLOPT_URL, ...) failed"
              << std::endl;
    curl_easy_cleanup(curl);
    return false;
  }
  std::clog << "using URL: " << url << std::endl;
  if (curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, body_cb) != CURLE_OK) {
    std::clog << "fatal: curl_easy_setopt(CURLOPT_WRITEFUNCTION, ...) failed"
              << std::endl;
    curl_easy_cleanup(curl);
    return false;
  }
  std::clog << "configured write callback" << std::endl;
  if (curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_body) != CURLE_OK) {
    std::clog << "fatal: curl_easy_setopt(CURLOPT_WRITEDATA, ...) failed"
              << std::endl;
    curl_easy_cleanup(curl);
    return false;
  }
  std::clog << "configured write callback context" << std::endl;
  std::clog << "cURL-performing HTTP request..." << std::endl;
  auto rv = curl_easy_perform(curl);
  std::clog << "cURL-performing HTTP request... done" << std::endl;
  if (rv != CURLE_OK) {
    std::clog << "fatal: curl_easy_perform() failed: " << curl_easy_strerror(rv)
              << std::endl;
    curl_easy_cleanup(curl);
    return false;
  }
  curl_easy_cleanup(curl);
  std::clog << "got this response body: " << response_body.str() << std::endl;
  nlohmann::json json;
  try {
    json = nlohmann::json::parse(response_body.str());
  } catch (const nlohmann::json::exception &) {
    std::clog << "fatal: nlohmann::json::parse() failed" << std::endl;
    return false;
  }
  std::clog << "successfully parsed body as JSON" << std::endl;

  for (auto result : json["results"]) {
    urls->push_back(result["url"]);
  }
  return true;
}

}  // namespace wac
}  // namespace mk
