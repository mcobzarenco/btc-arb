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

boost::optional<const Currency> ccy_from_string(const std::string& cyc) {
  string norm = cyc;
  transform(cyc.begin(), cyc.end(), norm.begin(), ::tolower);
  if (norm == "usd") {
    return boost::optional<const Currency>(Currency::USD);
  } else if (norm == "eur") {
    return boost::optional<const Currency>(Currency::EUR);
  }  else if (norm == "gbp") {
    return boost::optional<const Currency>(Currency::GBP);
  }  else if (norm == "jpy") {
    return boost::optional<const Currency>(Currency::JPY);
  } else {
    return boost::optional<const Currency>();
  }
}

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
