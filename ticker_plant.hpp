#pragma once

#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>
#include <json/value.h>

#include <functional>
#include <string>
#include <vector>


namespace btc_arb {

using TickHandler = std::function<void(const Json::Value&)>;
using TickFilter = std::function<bool(const Json::Value)>;

enum class EventType {
  ALL, QUOTES, TRADES, TICKER
};

class TickerPlant {
 public:
  virtual void add_tick_handler(TickHandler&& handler) = 0;
  virtual void run() = 0;
};

class WebSocketTickerPlant : public TickerPlant {
 public:
  using ws_client = websocketpp::client<websocketpp::config::asio_client>;
  using message_ptr = websocketpp::config::asio_client::message_type::ptr;

  WebSocketTickerPlant(const std::string& uri_);
  virtual void add_tick_handler(TickHandler&& handler) override;
  virtual void run() override;
 private:
  void dispatcher(websocketpp::connection_hdl hdl, message_ptr msg);

  const std::string uri_;
  ws_client client_;

  std::vector<TickHandler> handlers_;
};

}  // namespace btc_arb
