// Part of Measurement Kit <https://measurement-kit.github.io/>.
// Measurement Kit is free software under the BSD license. See AUTHORS
// and LICENSE for more information on the copying conditions.

#include "orchestrate.hpp"

#include <iostream>
#include <sstream>

#include <curl/curl.h>

#include "json.hpp"

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

static std::string make_url(
    const std::string &baseurl, const std::string &path,
    const std::map<std::string, std::string> &query) noexcept {
  // baseurl ex: https://orchestrate.ooni.io
  std::stringstream ss;
  ss << baseurl << "/api/v1" << path;
  if (query.empty()) {
    return ss.str();
  }

  ss << "?";
  std::vector<std::string> parts;
  for (auto &kv : query) {
    std::stringstream ss;
    ss << kv.first << "=" << kv.second;
    parts.push_back(ss.str());
  }
  std::copy(parts.begin(), parts.end(),
            std::ostream_iterator<std::string>(ss, "&"));
  // XXX this is a bit sketchy
  return ss.str().substr(0, ss.str().size() - 1);
}

namespace mk {
namespace wac {

bool OrchestrateClient::get_urls(const std::string &country_code,
                                 const std::vector<std::string> &category_codes,
                                 size_t limit,
                                 std::vector<std::string> *urls) noexcept {
  if (urls == nullptr) {
    return false;
  }

  // Ignore unused arguments
  (void)country_code, (void)category_codes, (void)limit;

  std::stringstream response_body;
  CURL *curl = curl_easy_init();
  if (curl == nullptr) {
    std::clog << "fatal: curl_easy_init() failed" << std::endl;
    return false;
  }
  std::clog << "cURL initialized" << std::endl;

  std::map<std::string, std::string> query;
  if (!country_code.empty()) {
    query["country_code"] = country_code;
  }
  if (!category_codes.empty()) {
    std::stringstream ss;
    std::copy(category_codes.begin(), category_codes.end(),
              std::ostream_iterator<std::string>(ss, ","));
    // XXX this is a bit sketchy
    query["category_codes"] = ss.str().substr(0, ss.str().size() - 1);
  }
  if (limit > 0) {
    query["limit"] = std::to_string(limit);
  }
  auto url = make_url("https://events.proteus.test.ooni.io", "/urls", query);

  if (curl_easy_setopt(curl, CURLOPT_URL, url.c_str()) != CURLE_OK) {
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

  try {
    for (auto result : json["results"]) {
      urls->push_back(result["url"]);
    }
  } catch (const nlohmann::json::exception &) {
    std::clog << "fatal: JSON processing failed" << std::endl;
    return false;
  }

  return true;
}

}  // namespace wac
}  // namespace mk
