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

static CURL *setup_curl(OrchestrateClient *client, const std::string &path,
                        const std::map<std::string, std::string> &query,
                        std::stringstream *response_body) noexcept {
  const OrchestrateSettings &settings = client->settings;

  CURL *curl = client->curl_easy_init();
  if (curl == nullptr) {
    std::clog << "fatal: curl_easy_init() failed" << std::endl;
    return nullptr;
  }
  std::clog << "cURL initialized" << std::endl;

  std::string url;
  switch (settings.type) {
    case BackendType::HTTPS: {
      std::stringstream ss;
      ss << "https://" << settings.address;
      url = make_url(ss.str(), path, query);
      break;
    }
    case BackendType::HTTP: {
      std::stringstream ss;
      ss << "http://" << settings.address;
      url = make_url(ss.str(), path, query);
      break;
    }
    case BackendType::ONION: {
      if (settings.socks_config.empty()) {
        return nullptr;
      }
      std::stringstream ss;
      ss << "http://" << settings.address;
      url = make_url(ss.str(), path, query);
      break;
    }
    case BackendType::DOMAIN_FRONTED: {
      std::stringstream ss;
      ss << "https://" << settings.front;
      url = make_url(ss.str(), path, query);

      struct curl_slist *list = NULL;
      std::stringstream header_ss;
      header_ss << "Host: " << settings.address;
      list = curl_slist_append(list, header_ss.str().c_str());
      if (list == nullptr) {
        curl_easy_cleanup(curl);
        return nullptr;
      }
      curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
      curl_slist_free_all(list);
      break;
    }
  }
  curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

  if (curl_easy_setopt(curl, CURLOPT_URL, url.c_str()) != CURLE_OK) {
    std::clog << "fatal: curl_easy_setopt(CURLOPT_URL, ...) failed"
              << std::endl;
    curl_easy_cleanup(curl);
    return nullptr;
  }
  std::clog << "using URL: " << url << std::endl;

  if (!settings.socks_config.empty()) {
    if (curl_easy_setopt(curl, CURLOPT_PROXY, settings.socks_config.c_str()) !=
        CURLE_OK) {
      curl_easy_cleanup(curl);
      return nullptr;
    }
  }

  if (curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, body_cb) != CURLE_OK) {
    std::clog << "fatal: curl_easy_setopt(CURLOPT_WRITEFUNCTION, ...) failed"
              << std::endl;
    curl_easy_cleanup(curl);
    return nullptr;
  }
  std::clog << "configured write callback" << std::endl;
  if (curl_easy_setopt(curl, CURLOPT_WRITEDATA, response_body) != CURLE_OK) {
    std::clog << "fatal: curl_easy_setopt(CURLOPT_WRITEDATA, ...) failed"
              << std::endl;
    curl_easy_cleanup(curl);
    return nullptr;
  }

  // XXX should this be the default?
  if (curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L) != CURLE_OK) {
    std::clog << "fatal: curl_easy_setopt(CURLOPT_FOLLOWLOCATION, ...) failed"
              << std::endl;
    curl_easy_cleanup(curl);
    return nullptr;
  }

  return curl;
}

bool OrchestrateClient::get_urls(const std::string &country_code,
                                 const std::vector<std::string> &category_codes,
                                 size_t limit,
                                 std::vector<std::string> *urls) noexcept {
  if (urls == nullptr) {
    return false;
  }

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

  std::stringstream response_body;
  CURL *curl = setup_curl(this, "/urls", query, &response_body);
  if (curl == nullptr) {
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

CURL *OrchestrateClient::curl_easy_init() noexcept {
  return ::curl_easy_init();
};

}  // namespace wac
}  // namespace mk
