#pragma once

#include "ticker_plant.hpp"
#include "market_logger.hpp"

#include <json/value.h>
#include <json/reader.h>
#include <json/writer.h>
#include <boost/optional.hpp>

#include <algorithm>
#include <cstdint>
#include <chrono>
#include <string>

#include <iostream>


namespace btc_arb {
namespace mtgox {

constexpr char CHANNEL_TRADES[] = "dbf1dee9-4f2e-4a08-8cb7-748919a71b21";
constexpr char CHANNEL_TICKER[] = "d5f06780-30a8-4a48-a2f8-7ed181b4a13f";
constexpr char CHANNEL_DEPTH[] = "24e67e0d-1cad-4cc0-9e7a-f8523ef460fe";

class FeedParser {
 protected:
  inline boost::optional<const Tick> parse(
      const std::string& msg, uint64_t received = 0);
 private:
  inline boost::optional<const Tick> parse_trade(
      const Json::Value& root, uint64_t received);

  inline boost::optional<const Tick> parse_depth(
      const Json::Value& root, uint64_t received);

  inline boost::optional<const Tick> parse_ticker(
      const Json::Value& root, uint64_t received);
};

boost::optional<const Tick> FeedParser::parse(
    const std::string& msg, uint64_t received) {
  Json::Value root;
  Json::Reader reader;
  Json::StyledWriter w;
  if (reader.parse(msg, root)) {
    // std::cout << w.write(root);
    if (received == 0) {
      received = std::chrono::system_clock::now().time_since_epoch().count();
    }
    const std::string tick_type = root.get("channel", "").asString();
    if (tick_type == CHANNEL_TRADES) {
      return parse_trade(root, received);
    } else if (tick_type == CHANNEL_DEPTH) {
      return parse_depth(root, received);
    } else if (tick_type == CHANNEL_TICKER) {
      return parse_ticker(root, received);
    } else {
      LOG(WARNING) << "Unknown channel \'" << tick_type << "\' in tick=" << msg;
      return boost::optional<const Tick>();
    }
  } else {
    LOG(WARNING) << "Could not parse tick (" << reader.getFormattedErrorMessages()
                 << ") raw tick=" << msg;
    return boost::optional<const Tick>();
  }
}

boost::optional<const Tick> FeedParser::parse_trade(
    const Json::Value& root, const uint64_t received) {
  try {
    const uint64_t ex_time{root["stamp"].asUInt64()};

    Trade::Type trade_type;
    std::string type_str{root["trade"].get("trade_type", "").asString()};
    std::transform(type_str.begin(), type_str.end(), type_str.begin(), ::tolower);
    if (type_str == "bid") {
      trade_type = Trade::Type::BID;
    } else if (type_str == "ask") {
      trade_type = Trade::Type::ASK;
    } else {
      throw std::runtime_error("unknown trade_type \'" + type_str + "\'");
    }

    const double amount{root["trade"].get("amount", 0.0).asDouble()};
    const int64_t amount_int{
      std::stoll(root["trade"].get("amount_int", 0).asString())};

    const std::string cyc_str{root["trade"].get("price_currency", "").asString()};
    boost::optional<const Currency> cyc_maybe =  ccy_from_string(cyc_str);
    if (!cyc_maybe) {
      throw std::runtime_error("unknown currency \'" + cyc_str + "\'");
    }
    const Currency cyc = *cyc_maybe;

    const double price{root["trade"].get("price", 0.0).asDouble()};
    const int32_t price_int{std::stoi(root["trade"].get("price_int", 0).asString())};
    return boost::optional<const Tick>(Trade{
        received, ex_time, trade_type, amount, amount_int, cyc, price, price_int});
  } catch (const std::exception& e) {
    Json::FastWriter writer;
    LOG(WARNING) << "Could not parse trade (" << e.what()
                 << ") json=" << writer.write(root);
    return boost::optional<const Tick>();
  }
}

boost::optional<const Tick> FeedParser::parse_depth(const Json::Value& root,
                                              const uint64_t received) {
  try {
    const uint64_t ex_time{root["stamp"].asUInt64()};
    const Json::Value& depth{root["depth"]};
    const int quote_type_int{depth.get("type", 0).asInt()};
    Quote::Type quote_type;
    if (quote_type_int == 1) {
      quote_type = Quote::Type::ASK_UPDATE;
    } else if (quote_type_int == 2) {
      quote_type = Quote::Type::BID_UPDATE;
    } else {
      throw std::runtime_error(
          "unknown quote type \'" + std::to_string(quote_type_int) + "\'");
    }
    const double delta_volume{std::stod(depth.get("volume", "").asString())};
    const int64_t delta_volume_int{
      std::stoll(depth.get("volume_int", "").asString())};
    const int64_t total_volume_int{
      std::stoll(depth.get("total_volume_int", "").asString())};
    const double total_volume{
      static_cast<double>(total_volume_int) / VOLUME_MULTIPLIER};

    const std::string cyc_str{root["depth"].get("currency", "").asString()};
    boost::optional<const Currency> cyc_maybe =  ccy_from_string(cyc_str);
    if (!cyc_maybe) {
      throw std::runtime_error("unknown currency \'" + cyc_str + "\'");
    }
    const Currency cyc = *cyc_maybe;

    const double price{std::stod(depth.get("price", "").asString())};
    const int32_t price_int{std::stoi(depth.get("price_int", "").asString())};
    return boost::optional<const Tick>(Quote{received, ex_time, quote_type,
            delta_volume, delta_volume_int, total_volume, total_volume_int,
            cyc, price, price_int});
  } catch (const std::exception& e) {
    Json::FastWriter writer;
    LOG(WARNING) << "Could not parse depth (" << e.what()
                 << ") json=" << writer.write(root);
    return boost::optional<const Tick>();
  }
}

boost::optional<const Tick> FeedParser::parse_ticker(const Json::Value& root,
                                                     uint64_t received) {
  return boost::optional<const Tick>();
}


}}  // namespace btc_arb::mtgox
