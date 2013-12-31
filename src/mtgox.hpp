#pragma once

#include "ticker_plant.hpp"
#include "market_logger.hpp"

#include <json/value.h>
#include <json/reader.h>
#include <boost/optional.hpp>

#include <cstdint>
#include <chrono>
#include <string>


namespace btc_arb {
namespace mtgox {

constexpr char CHANNEL_TRADES[] = "dbf1dee9-4f2e-4a08-8cb7-748919a71b21";
constexpr char CHANNEL_TICKER[] = "d5f06780-30a8-4a48-a2f8-7ed181b4a13f";
constexpr char CHANNEL_DEPTH[] = "24e67e0d-1cad-4cc0-9e7a-f8523ef460fe";

class FeedParser {
 protected:
  inline boost::optional<Tick> parse(const std::string& msg) {
    Json::Value root;
    Json::Reader reader;
    if (reader.parse(msg, root)) {
      const std::string tick_type = root.get("channel", "").asString();
      if (tick_type == CHANNEL_TRADES) {
        // LOG(INFO) << "got trade";
        const uint64_t received =
            std::chrono::system_clock::now().time_since_epoch().count();
        const uint64_t ex_time = root["depth"].asUInt64();
        const double price = root["depth"].get("price", 0.0).asDouble();
        const int price_int = root["depth"].get("price_int", 0).asInt();
        return boost::optional<Tick>(Trade(received, ex_time, price, price_int));
      } else if (tick_type == CHANNEL_DEPTH ||
                 tick_type == CHANNEL_TICKER) {
        // LOG(INFO) << "depth";
        return boost::optional<Tick>();
      } else {
        return boost::optional<Tick>();
      }
    } else {
      LOG(WARNING) << "Could not parse tick (" << reader.getFormattedErrorMessages()
                   << ") raw tick=" << msg;
      return boost::optional<Tick>();
    }
  }
};



}}  // namespace btc_arb::mtgox
