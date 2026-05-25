#include "event_sink/HttpEventSink.hpp"
#include "discovery/DiscoveryTypes.hpp"
#include "discovery/MacResolver.hpp"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <sstream>
#include <thread>

#if defined(LAN_HTTP_USE_CURL)
#include <curl/curl.h>
#elif defined(LAN_HTTP_USE_HTTPLIB)
#include "httplib.h"
#endif

namespace {

#if defined(LAN_HTTP_USE_CURL)
size_t curl_write_discard(char * /*ptr*/, size_t size, size_t nmemb,
                          void * /*userdata*/) {
  return size * nmemb;
}
#endif

bool parse_post_url(const std::string &post_url, std::string &scheme,
                    std::string &host, int &port, std::string &path) {
  const auto scheme_end = post_url.find("://");
  if (scheme_end == std::string::npos)
    return false;
  scheme = post_url.substr(0, scheme_end);
  std::string rest = post_url.substr(scheme_end + 3);
  const auto path_start = rest.find('/');
  const std::string host_port =
      path_start == std::string::npos ? rest : rest.substr(0, path_start);
  path = path_start == std::string::npos ? "/" : rest.substr(path_start);
  const auto colon = host_port.rfind(':');
  if (colon != std::string::npos && colon + 1 < host_port.size()) {
    host = host_port.substr(0, colon);
    try {
      port = std::stoi(host_port.substr(colon + 1));
    } catch (...) {
      return false;
    }
  } else {
    host = host_port;
    port = (scheme == "https") ? 443 : 80;
  }
  return !host.empty();
}

} // namespace

HttpEventSinkConfig make_http_sink_config(const ApiConfig &api) {
  HttpEventSinkConfig config;
  config.post_url = api.base_url + "/v1/discovery/events";
  config.api_token = api.api_token;
  return config;
}

class HttpEventSink : public EventSink {
public:
  HttpEventSink(HttpEventSinkConfig config, std::shared_ptr<Logger> logger)
      : config_(std::move(config)), logger_(std::move(logger)),
        worker_(&HttpEventSink::poster_loop, this) {}

  ~HttpEventSink() override {
    {
      std::lock_guard<std::mutex> lock(mutex_);
      stop_ = true;
    }
    cv_.notify_all();
    if (worker_.joinable())
      worker_.join();
  }

  void on_event(const DiscoveryEvent &ev) override {
    try {
      std::lock_guard<std::mutex> lock(mutex_);
      if (stop_)
        return;
      if (queue_.size() >= config_.max_queue_size) {
        queue_.pop_front();
        if (logger_)
          logger_->warn("http queue full, dropped oldest event");
      }
      queue_.push_back(ev);
      cv_.notify_one();
    } catch (const std::exception &ex) {
      if (logger_) {
        logger_->warn(std::string("http enqueue failed: ") + ex.what());
      }
    }
  }

private:
  HttpEventSinkConfig config_;
  std::shared_ptr<Logger> logger_;
  std::mutex mutex_;
  std::condition_variable cv_;
  std::deque<DiscoveryEvent> queue_;
  bool stop_ = false;
  std::thread worker_;

  void poster_loop() {
    while (true) {
      DiscoveryEvent ev;
      {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this] { return stop_ || !queue_.empty(); });
        if (stop_ && queue_.empty())
          return;
        ev = std::move(queue_.front());
        queue_.pop_front();
      }

      int backoff_ms = 1000;
      constexpr int k_max_backoff_ms = 60'000;
      for (;;) {
        if (post_once(ev))
          break;
        if (logger_) {
          std::ostringstream oss;
          oss << "http post failed, retry in " << backoff_ms << " ms";
          logger_->warn(oss.str());
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(backoff_ms));
        backoff_ms = std::min(backoff_ms * 2, k_max_backoff_ms);
        {
          std::lock_guard<std::mutex> lock(mutex_);
          if (stop_)
            return;
        }
      }
    }
  }

  bool post_once(const DiscoveryEvent &ev) {
    const auto mac = MacResolver::lookup(ev.src_ip);
    const std::string body = to_json(ev, mac);

#if defined(LAN_HTTP_USE_CURL)
    CURL *curl = curl_easy_init();
    if (!curl) {
      if (logger_)
        logger_->error("http post: curl_easy_init failed");
      return false;
    }

    struct curl_slist *headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    std::string api_key_header;
    if (!config_.api_token.empty()) {
      api_key_header = "X-API-Key: " + config_.api_token;
      headers = curl_slist_append(headers, api_key_header.c_str());
    }

    curl_easy_setopt(curl, CURLOPT_URL, config_.post_url.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE,
                     static_cast<long>(body.size()));
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, config_.http_timeout_sec);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_discard);

    const CURLcode res = curl_easy_perform(curl);
    long http_code = 0;
    if (res == CURLE_OK)
      curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
      if (logger_) {
        logger_->warn(std::string("http post curl error: ") +
                      curl_easy_strerror(res));
      }
      return false;
    }
    if (http_code < 200 || http_code >= 300) {
      if (logger_) {
        std::ostringstream oss;
        oss << "http post rejected, status=" << http_code;
        logger_->warn(oss.str());
      }
      return false;
    }
    return true;

#elif defined(LAN_HTTP_USE_HTTPLIB)
    std::string scheme;
    std::string host;
    std::string path;
    int port = 80;
    if (!parse_post_url(config_.post_url, scheme, host, port, path)) {
      if (logger_)
        logger_->warn("http post: invalid URL " + config_.post_url);
      return false;
    }

    httplib::Headers headers = {{ "Content-Type", "application/json" }};
    if (!config_.api_token.empty()) {
      headers.emplace("X-API-Key", config_.api_token);
    }

    const auto timeout =
        std::chrono::seconds(static_cast<int>(config_.http_timeout_sec));
    if (scheme == "https") {
      if (logger_) {
        logger_->warn(
            "https API URLs require libcurl (install libcurl4-openssl-dev)");
      }
      return false;
    }

    httplib::Client client(host, port);
    client.set_connection_timeout(timeout);
    client.set_read_timeout(timeout);
    client.set_write_timeout(timeout);
    const httplib::Result result =
        client.Post(path, headers, body, "application/json");

    if (!result) {
      if (logger_)
        logger_->warn("http post failed: connection error");
      return false;
    }
    if (result->status < 200 || result->status >= 300) {
      if (logger_) {
        std::ostringstream oss;
        oss << "http post rejected, status=" << result->status;
        logger_->warn(oss.str());
      }
      return false;
    }
    return true;
#else
    (void)body;
    if (logger_)
      logger_->error("http post: no HTTP backend compiled");
    return false;
#endif
  }
};

std::shared_ptr<EventSink>
make_http_event_sink(HttpEventSinkConfig config,
                     std::shared_ptr<Logger> logger) {
  return std::make_shared<HttpEventSink>(std::move(config), std::move(logger));
}
