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
#include <string>
#include <functional>
#include <chrono>


using namespace std;
using namespace btc_arb;

int main(int argc, char **argv) {
  namespace po = boost::program_options;
  google::InitGoogleLogging(argv[0]);
  google::LogToStderr();

  string uri = "ws://websocket.mtgox.com/mtgox";

  auto build_time = string{__TIMESTAMP__};

  auto description = po::options_description{
    "Ticker Plant (built on " + build_time +
    ") persists market data/backtests/runs live strategies.\n\nAllowed options:"
  };
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

    LevelDBLogger market_logger("mark.leveldb");
    auto tick2db = bind(&LevelDBLogger::log_tick, &market_logger, placeholders::_1);

    unique_ptr<TickerPlant> plant{new WebSocketTickerPlant(uri)};
    plant->add_tick_handler(tick2db);
    plant->add_tick_handler(report_progress_mtgox);
    // plant->add_tick_handler(report_progress);
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
