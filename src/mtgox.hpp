#pragma once

#include "ticker_plant.hpp"

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
  inline boost::optional<const ParsedTick> parse(
      std::istream& stream, uint64_t received = 0);
 private:
  inline boost::optional<Tick> parse_trade(
      const Json::Value& root, uint64_t received);
  inline boost::optional<Tick> parse_depth(
      const Json::Value& root, uint64_t received);
  inline boost::optional<Tick> parse_ticker(
      const Json::Value& root, uint64_t received);

  template<typename EnumType>
  inline EnumType str_to_enum(std::string str);
};

boost::optional<const ParsedTick> FeedParser::parse(
    std::istream& stream, uint64_t received) {
  Json::Value root;
  Json::Reader reader;
  Json::FastWriter writer;
  Json::StyledWriter w;
  std::string msg;
  std::getline(stream, msg);
  if (reader.parse(msg, root)) {
    // std::cout << w.write(root);
    if (received == 0) {
      received = std::chrono::system_clock::now().time_since_epoch().count();
    }
    if(root.get("_received", 0).asUInt64() == 0) {
      root["_received"] = Json::Value{static_cast<Json::UInt64>(received)};
    }
    const std::string tick_type = root.get("channel", "").asString();
    boost::optional<Tick> tick;
    if (tick_type == CHANNEL_TRADES) {
      tick = parse_trade(root, received);
    } else if (tick_type == CHANNEL_DEPTH) {
      tick = parse_depth(root, received);
    } else if (tick_type == CHANNEL_TICKER) {
      tick = parse_ticker(root, received);
    } else {
      LOG(WARNING) << "Unknown channel \'" << tick_type << "\' in tick=" << msg;
    }
    if(tick) {
      return boost::optional<const ParsedTick>(ParsedTick{*tick, writer.write(root)});
    }
    return boost::optional<const ParsedTick>();
  } else {
    LOG(WARNING) << "Could not parse tick (" << reader.getFormattedErrorMessages()
                 << ") raw tick=" << msg;
    return boost::optional<const ParsedTick>();
  }
}

boost::optional<Tick> FeedParser::parse_trade(
    const Json::Value& root, const uint64_t received) {
  try {
    const uint64_t ex_time{root["stamp"].asUInt64()};
    Trade::Type trade_type = str_to_enum<Trade::Type>(
        root["trade"].get("trade_type", "").asString());
    const double amount{root["trade"].get("amount", 0.0).asDouble()};
    const int64_t amount_int{
      std::stoll(root["trade"].get("amount_int", 0).asString())};
    const Currency cyc(str_to_enum<Currency>(
        root["trade"].get("price_currency", "").asString()));
    const double price{root["trade"].get("price", 0.0).asDouble()};
    const int32_t price_int{std::stoi(root["trade"].get("price_int", 0).asString())};
    return boost::optional<Tick>(Trade{
        received, ex_time, trade_type, amount, amount_int, cyc, price, price_int});
  } catch (const std::exception& e) {
    Json::FastWriter writer;
    LOG(WARNING) << "Could not parse trade (" << e.what()
                 << ") json=" << writer.write(root);
    return boost::optional<Tick>();
  }
}

boost::optional<Tick> FeedParser::parse_depth(const Json::Value& root,
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
    const Currency cyc(str_to_enum<Currency>(
        root["depth"].get("currency", "").asString()));
    const double price{std::stod(depth.get("price", "").asString())};
    const int32_t price_int{std::stoi(depth.get("price_int", "").asString())};
    return boost::optional<Tick>(Quote{received, ex_time, quote_type,
            delta_volume, delta_volume_int, total_volume, total_volume_int,
            cyc, price, price_int});
  } catch (const std::exception& e) {
    Json::FastWriter writer;
    LOG(WARNING) << "Could not parse depth (" << e.what()
                 << ") json=" << writer.write(root);
    return boost::optional<Tick>();
  }
}

boost::optional<Tick> FeedParser::parse_ticker(const Json::Value& root,
                                               uint64_t received) {
  return boost::optional<Tick>();
}

template<typename EnumType>
EnumType FeedParser::str_to_enum(std::string str) {
  EnumType enum_type;
  std::transform(str.begin(), str.end(), str.begin(), ::tolower);
  std::stringstream type_stream{str, std::ios::in};
  type_stream >> enum_from_str(enum_type);
  return enum_type;
}

}}  // namespace btc_arb::mtgox
