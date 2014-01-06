#include "ticker_plant.hpp"
#include "market_logger.hpp"
#include "log_reporter.hpp"
#include "mtgox.hpp"
#include "enum_utils.hpp"

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
#include <stdexcept>
#include <vector>


using namespace std;
using namespace btc_arb;

namespace {
enum class SourceType { FLAT, LDB, WS_MTGOX, LDB_MTGOX };
enum class SinkType { FLAT, RAW_LDB };
}

template<> char const* utils::EnumStrings<SourceType>::names[] = {
  "flat", "ldb", "ws_mtgox", "ldb_mtgox"};
template<> char const* utils::EnumStrings<SinkType>::names[] = {
  "flat", "raw_ldb"};

namespace {
template<typename T>
struct PrependedPath {
  static PrependedPath parse(const string& spath);

  T type;
  string path;
};

template<typename T>
PrependedPath<T> PrependedPath<T>::parse(const string& spath) {
  string::size_type delim{spath.find(':')};
  if (delim == string::npos) {
    throw runtime_error("invalid path \'" + spath + "\'");
  }
  stringstream type_str{spath.substr(0, delim)};
  T type;
  type_str >> utils::enum_from_str(type);
  string path{spath.substr(delim + 1)};
  return PrependedPath<T>{move(type), move(path)};
}

template<typename T>
vector<PrependedPath<T>> parse_all(const vector<string>& s) {
  vector<PrependedPath<T>> parsed(s.size());
  transform(s.begin(), s.end(), parsed.begin(), PrependedPath<T>::parse);
  return parsed;
}
}  // anonymous namespace

int main(int argc, char **argv) {
  namespace po = boost::program_options;
  namespace ph = std::placeholders;
  google::InitGoogleLogging(argv[0]);
  google::LogToStderr();

  string source_str{"ws_mtgox:ws://websocket.mtgox.com/mtgox"};

  stringstream desc_msg;
  desc_msg << "Ticker Plant -- persists market data and runs strategies "
           << "(live or backtest) " << endl
           << "Built on " << __TIMESTAMP__ << endl << endl
           << "usage: " << argv[0] << " [CONFIG] <SOURCE>" << endl << endl
           << "Allowed options:";

  auto description = po::options_description{desc_msg.str()};
  description.add_options()
      ("help,h", "prints this help message")
      ("source",
       po::value<string>(&source_str)->value_name("TYPE:PATH"),
       ("the market data souce; can also be specified as a positional arg; "
        "available types: flat, ldb, ws_mtgox, ldb_mtgox; "
        "default=" + source_str).c_str())
      ("sink",
       po::value<vector<string>>()->value_name("TYPE:PATH"),
       "specifies a sink for the ticks; available types: flat, raw_ldb");
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

    if (variables.count("sink")) {
      auto sinks = variables["sink"].as<vector<string>>();
      cout << "sinks: ";
      for (auto& s : sinks) {
        cout << s << " ";
      }
      cout << endl;

      auto parsed = parse_all<SinkType>(sinks);
      for_each(parsed.begin(), parsed.end(),
               [](const PrependedPath<SinkType> &p) {
                 cout << "sink " << utils::enum_to_str(p.type) << " " << p.path << endl;
               });
    }

    PrependedPath<SourceType> spath = PrependedPath<SourceType>::parse(source_str);
    cout << "::: " << (utils::enum_to_str(spath.type)) << endl;
    unique_ptr<TickerPlant> plant{nullptr};
    switch (spath.type) {
      case SourceType::FLAT:
        plant.reset(new FlatFileTickerPlant(spath.path));
        break;
      case SourceType::LDB:
        plant.reset(new LdbTickerPlant<FlatParser>(spath.path));
        break;
      case SourceType::WS_MTGOX:
        plant.reset(new WebSocketTickerPlant<mtgox::FeedParser>(spath.path));
        break;
      case SourceType::LDB_MTGOX:
        plant.reset(new LdbTickerPlant<mtgox::FeedParser>(spath.path));
        break;
    }

    // LevelDBLogger market_log("market.leveldb");
    // LevelDBLogger trades_log("trades.leveldb", only_trades);
    vector<LdbLogger> raw_loggers;
    vector<FileLogger> file_loggers;
    FileLogger mlog("out.flat");

    cout << static_cast<int>(spath.type) << " " << spath.path << endl;

    // unique_ptr<TickerPlant> plant{new WebSocketTickerPlant(uri)};
    // plant->add_tick_handler(bind(&LevelDBLogger::log_tick, &market_log, ph::_1));
    // plant->add_tick_handler(bind(&LevelDBLogger::log_tick, &trades_log, ph::_1));
    // plant->add_tick_handler(bind(&FlatFileLogger::log_tick, &mlog, ph::_1));
    plant->add_tick_handler(report_progress_block);
    plant->add_tick_handler([&mlog](const Tick& tick) {
        // if (tick.type == Tick::Type::QUOTE) {
        //   const Quote& quote = tick.as<Quote>();
        //   LOG(INFO) << "Q " << quote.received << " " << quote.ex_time << " lag="
        //             << (quote.received - quote.ex_time)
        //             << " " << quote.total_volume << " @ " << quote.price;

        // } else if (tick.type == Tick::Type::TRADE) {
        //   const Trade& trade = tick.as<Trade>();
        // }
        // cout << static_cast<int>(tick.type) << endl;
        // mlog.log(reinterpret_cast<const char *>(&tick), sizeof(Tick));
      });
    LOG (INFO) << "starting ticker plant";
    // plant->run();
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
