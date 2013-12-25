#include "ticker_plant.hpp"

#include <glog/logging.h>
#include <json/reader.h>


namespace btc_arb {

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

using namespace std;

WebSocketTickerPlant::WebSocketTickerPlant(const string& uri)
    : uri_(uri) {
  client_.init_asio();
}

void WebSocketTickerPlant::add_tick_handler(TickHandler&& handler) {
  handlers_.emplace_back(handler);
}

void WebSocketTickerPlant::run() {
  client_.set_message_handler(
      bind(&WebSocketTickerPlant::dispatcher, this, _1, _2));

  websocketpp::lib::error_code ec;
  ws_client::connection_ptr conn = client_.get_connection(uri_, ec);
  conn->replace_header("Origin", uri_);
  client_.connect(conn);
  client_.run();
}

void WebSocketTickerPlant::dispatcher(
    websocketpp::connection_hdl hdl, message_ptr msg) {
  const std::string& tick = msg->get_payload();
  Json::Value root;
  Json::Reader reader;
  if (!reader.parse(tick, root)) {
    LOG(WARNING) << "Could not parse tick (" << reader.getFormattedErrorMessages()
                 << ") raw tick=" << msg;
  }

  for(auto& handler : handlers_) {
    handler(root);
  }
}

}  // namespace btc_arb
