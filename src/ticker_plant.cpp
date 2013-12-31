#include "ticker_plant.hpp"

#include <glog/logging.h>
#include <json/reader.h>

#include <iostream>
#include <sstream>


namespace btc_arb {

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

using namespace std;

void TickerPlant::add_tick_handler(TickHandler&& handler) {
  handlers_.emplace_back(handler);
}


FlatFileTickerPlant::FlatFileTickerPlant(const std::string& path_to_file) {
    file_.open(path_to_file, std::ios::in | std::ios::binary);
}

bool FlatFileTickerPlant::run() {
    CHECK (file_.is_open()) << "file not open";
    Tick tick;
    while (file_.read(reinterpret_cast<char*>(&tick), sizeof(Tick))) {
        call_handlers(tick);
    }
    return true;
}


}  // namespace btc_arb
