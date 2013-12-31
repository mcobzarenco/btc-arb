#include "ticker_plant.hpp"
#include "market_logger.hpp"
#include "log_reporter.hpp"
#include "mtgox.hpp"

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

enum class SourceType {
  WEBSOCKET, LEVELDB, FLAT_JSON
};

SourceType get_source_type(const string& arg) {
  if (arg == "ldb") {
    return SourceType::LEVELDB;
  } else if (arg == "ws") {
    return SourceType::WEBSOCKET;
  } else if (arg == "flat_json") {
    return SourceType::FLAT_JSON;
  } else {
    throw boost::program_options::invalid_option_value(arg);
  }
}

int main(int argc, char **argv) {
  namespace po = boost::program_options;
  namespace ph = std::placeholders;
  google::InitGoogleLogging(argv[0]);
  google::LogToStderr();

  SourceType source_type{SourceType::WEBSOCKET};
  string source{"ws://websocket.mtgox.com/mtgox"};

  stringstream desc_msg;
  desc_msg << "Ticker Plant -- persists market data and runs strategies "
           << "(live or backtest) " << endl
           << "Built on " << __TIMESTAMP__ << endl << endl
           << "usage: " << argv[0] << " [CONFIG] <SOURCE>" << endl << endl
           << "Allowed options:";

  auto description = po::options_description{desc_msg.str()};
  description.add_options()
      ("help,h", "prints this help message")
      ("src-type,t",
       po::value<string>()->value_name("T"),
       "specifies the source type (available: ldb, ws, flat_json)")
      ("source",
       po::value<string>(&source)->value_name("SRC"),
       ("the market data souce; can also be specified as a positional arg; "
        "default=" + source).c_str());
  po::positional_options_description positional;
  positional.add("source", -1);

  auto variables = po::variables_map{};
  try {
    po::store(po::command_line_parser(argc, argv)
              .options(description).positional(positional).run(), variables);
    po::notify(variables);
    if (variables.count("help")) {
      cerr << description << endl;
      return 0;
    }
    if (variables.count("src-type")) {
      source_type = get_source_type(variables["src-type"].as<string>());
    }

    auto only_trades = [](const Json::Value& root) {
        return root["channel_name"] == "trade.BTC";
    };

    unique_ptr<TickerPlant> plant{nullptr};
    switch (source_type) {
      case SourceType::LEVELDB:
        plant.reset(new LdbTickerPlant<mtgox::FeedParser>(source));
        break;
      case SourceType::WEBSOCKET:
          plant.reset(new WebSocketTickerPlant<mtgox::FeedParser>(source));
        break;
      case SourceType::FLAT_JSON:
        plant.reset(new FlatFileTickerPlant(source));
        break;
    }
    // LevelDBLogger market_log("market.leveldb");
    // LevelDBLogger trades_log("trades.leveldb", only_trades);
    FileLogger mlog("mtgox2.flat");

    // unique_ptr<TickerPlant> plant{new WebSocketTickerPlant(uri)};
    // plant->add_tick_handler(bind(&LevelDBLogger::log_tick, &market_log, ph::_1));
    // plant->add_tick_handler(bind(&LevelDBLogger::log_tick, &trades_log, ph::_1));
    // plant->add_tick_handler(bind(&FlatFileLogger::log_tick, &mlog, ph::_1));
    // plant->add_tick_handler(mtgox::report_progress_block);
    plant->add_tick_handler([&mlog](const Tick& tick) {
        cout << static_cast<int>(tick.type) << endl;
        // mlog.log(reinterpret_cast<const char *>(&tick));
      });
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
