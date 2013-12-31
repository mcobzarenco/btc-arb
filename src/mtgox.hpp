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
  inline boost::optional<Tick> parse(
      const std::string& msg, uint64_t received = 0);
 private:
  inline boost::optional<Tick> parse_trade(
      const Json::Value& root, uint64_t received);

  inline boost::optional<Tick> parse_depth(
      const Json::Value& root, uint64_t received);

  inline boost::optional<Tick> parse_ticker(
      const Json::Value& root, uint64_t received);
};

boost::optional<Tick> FeedParser::parse(
    const std::string& msg, uint64_t received) {
  Json::Value root;
  Json::Reader reader;
  Json::StyledWriter w;
  if (reader.parse(msg, root)) {
    std::cout << w.write(root);
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
      return boost::optional<Tick>();
    }
  } else {
    LOG(WARNING) << "Could not parse tick (" << reader.getFormattedErrorMessages()
                 << ") raw tick=" << msg;
    return boost::optional<Tick>();
  }
}

boost::optional<Tick> FeedParser::parse_trade(
    const Json::Value& root, uint64_t received) {
  const uint64_t ex_time = root["stamp"].asUInt64();
  const double amount = root["trade"].get("amount", 0.0).asDouble();
  const uint64_t amount_int = root["trade"].get("amount_int", 0).asUInt64();

  Currency cyc;
  std::string cyc_str = root["trade"].get("price_currency", "").asString();
  std::transform(cyc_str.begin(), cyc_str.end(), cyc_str.begin(), ::tolower);
  if (cyc_str == "usd") {
    cyc = Currency::USD;
  } else if (cyc_str == "eur") {
    cyc = Currency::EUR;
  }  else if (cyc_str == "gbp") {
    cyc = Currency::GBP;
  }  else if (cyc_str == "jpy") {
    cyc = Currency::JPY;
  } else {
    LOG(WARNING) << "Unknown currency \'" << cyc_str << "\'";
    return boost::optional<Tick>();
  }

  const double price = root["trade"].get("price", 0.0).asDouble();
  const uint32_t price_int = root["trade"].get("price_int", 0).asUInt();

  QuoteType trade_type;
  std::string type_str = root["trade"].get("type", "").asString();
  std::transform(type_str.begin(), type_str.end(), type_str.begin(), ::tolower);
  if (type_str == "bid") {
    trade_type = QuoteType::BID;
  } else if (type_str == "ask") {
    trade_type = QuoteType::ASK;
  } else {
    LOG(WARNING) << "Unknown trade_type \'" << type_str << "\'";
    return boost::optional<Tick>();
  }

  return boost::optional<Tick>(Trade{
      received, ex_time, amount, amount_int,
          cyc, price, price_int, trade_type});
}

boost::optional<Tick> FeedParser::parse_depth(const Json::Value& root,
                                              uint64_t received) {
  const uint64_t ex_time = root["depth"].asUInt64();
  const double amount = root["trade"].get("amount", 0.0).asDouble();
  const uint64_t amount_int = root["trade"].get("amount_int", 0).asUInt64();


  const double price = root["depth"].get("price", 0.0).asDouble();
  const uint32_t price_int = root["depth"].get("price_int", 0).asInt();
  return boost::optional<Tick>();
}

boost::optional<Tick> FeedParser::parse_ticker(const Json::Value& root,
                                               uint64_t received) {
  return boost::optional<Tick>();
}


}}  // namespace btc_arb::mtgox
