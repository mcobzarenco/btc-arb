#include "ticker_plant.hpp"

#include <glog/logging.h>
#include <json/reader.h>

#include <iostream>
#include <sstream>
#include <string>


namespace btc_arb {

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

using namespace std;

constexpr const char* EnumStrings<Currency>::names[];
constexpr const char* EnumStrings<Quote::Type>::names[];
constexpr const char* EnumStrings<Trade::Type>::names[];

void TickerPlant::add_tick_handler(TickHandler&& handler) {
  handlers_.emplace_back(move(handler));
}

void TickerPlant::add_raw_handler(RawHandler&& handler) {
  raw_handlers_.emplace_back(move(handler));
}

FileLogger::FileLogger(const std::string& path_to_file) {
  file_.reset(new std::ofstream());
  file_->open(path_to_file, std::ios::out | std::ios::app);
}

}  // namespace btc_arb
