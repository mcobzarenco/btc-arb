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

void TickerPlant::add_tick_handler(TickHandler&& handler) {
  handlers_.emplace_back(handler);
}

template<typename Parser>
FileTickerPlant<Parser>::FileTickerPlant(const std::string& path_to_file) {
  file_.open(path_to_file, std::ios::in | std::ios::binary);
}

template<typename Parser>
bool FileTickerPlant<Parser>::run() {
  CHECK (file_.is_open()) << "file not open";
  boost::optional<const ParsedTick> parsed;
  while (file_) {
    parsed = Parser::parse(file_);
    if (parsed) {
      call_handlers((*parsed).tick);
    }
  }
  return true;
}


}  // namespace btc_arb
