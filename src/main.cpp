#include "ticker_plant.hpp"
#include "market_logger.hpp"
#include "log_reporter.hpp"

#include <boost/program_options.hpp>
#include <glog/logging.h>
#include <json/value.h>
#include <json/reader.h>

#include <iostream>
#include <iomanip>
#include <memory>
#include <cstdlib>
#include <functional>
#include <string>
#include <sstream>


using namespace std;
using namespace btc_arb;

int main(int argc, char **argv) {
  namespace po = boost::program_options;
  namespace ph = std::placeholders;
  google::InitGoogleLogging(argv[0]);
  google::LogToStderr();

  string uri = "ws://websocket.mtgox.com/mtgox";
  stringstream description_msg;
  description_msg << "Ticker Plant (built on " << __TIMESTAMP__
                  << ") persists market data/backtests/runs "
                  << "live strategies.\n\nAllowed options:";

  auto build_time = string{__TIMESTAMP__};
  auto description = po::options_description{};
  description.add_options()
      ("help,h", "prints this help message")
      ("uri",
       po::value<string>(&uri)->value_name("URI"),
       "Config file. Options in cmd line overwrite the values from the file.");
  po::positional_options_description positional;
  positional.add("uri", -1);

  auto variables = po::variables_map{};

  try {
    po::store(po::command_line_parser(argc, argv)
              .options(description).positional(positional).run(), variables);
    po::notify(variables);
    if (variables.count("help")) {
      cerr << description << endl;
      return 0;
    }

    auto only_trades = [](const Json::Value& root) {
        return root["channel_name"] == "trade.BTC";
    };
    LevelDBLogger market_log("market.leveldb");
    LevelDBLogger trades_log("trades.leveldb", only_trades);

    unique_ptr<TickerPlant> plant{new WebSocketTickerPlant(uri)};
    plant->add_tick_handler(bind(&LevelDBLogger::log_tick, &market_log, ph::_1));
    plant->add_tick_handler(bind(&LevelDBLogger::log_tick, &trades_log, ph::_1));
    plant->add_tick_handler(report_progress_mtgox);
    plant->run();
  } catch (const boost::program_options::unknown_option& e) {
    LOG(ERROR) << e.what();
    return 1;
  } catch (const boost::program_options::invalid_option_value& e) {
    LOG(ERROR) << e.what();
    return 2;
  } catch(const std::exception& e) {
    LOG(ERROR) << e.what();
    return -1;
  }
}
