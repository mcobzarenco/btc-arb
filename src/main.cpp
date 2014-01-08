#include "ticker_plant.hpp"
#include "log_reporter.hpp"
#include "mtgox.hpp"
#include "enum_utils.hpp"

#include <boost/program_options.hpp>
#include <glog/logging.h>

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
enum class SourceType { FLAT, FLAT_MTGOX, WS_MTGOX };
enum class SinkType { FLAT, FLAT_RAW };
}

template<> char const* utils::EnumStrings<SourceType>::names[] = {
  "flat", "flat_mtgox", "ws_mtgox"};
template<> char const* utils::EnumStrings<SinkType>::names[] = {
  "flat", "flat_raw"};

namespace {
template<typename T>
struct PrependedPath {
  static PrependedPath parse(const string& spath);
  static vector<PrependedPath<T>> parse_all(const vector<string>& s);

  T type;
  string path;
};

template<typename T>
PrependedPath<T> PrependedPath<T>::parse(const string& spath) {
  string::size_type delim{spath.find(':')};
  if (delim == string::npos) {
    throw runtime_error("invalid path \'" + spath + "\'");
  }
  stringstream type_str{spath.substr(0, delim), ios::in};
  string path{spath.substr(delim + 1)};
  T type;
  type_str >> utils::enum_from_str(type);
  return PrependedPath<T>{move(type), move(path)};
}

template<typename T>
vector<PrependedPath<T>> PrependedPath<T>::parse_all(const vector<string>& s) {
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

    PrependedPath<SourceType> spath = PrependedPath<SourceType>::parse(source_str);
    unique_ptr<TickerPlant> plant{nullptr};
    switch (spath.type) {
      case SourceType::FLAT:
        plant.reset(new FileTickerPlant<FlatParser>(spath.path));
        break;
      case SourceType::FLAT_MTGOX:
        plant.reset(new FileTickerPlant<mtgox::FeedParser>(spath.path));
        break;
      case SourceType::WS_MTGOX:
        plant.reset(new WebSocketTickerPlant<mtgox::FeedParser>(spath.path));
        break;
    }

    vector<FileLogger> loggers;
    if (variables.count("sink")) {
      auto sinks = PrependedPath<SinkType>::parse_all(
          variables["sink"].as<vector<string>>());
      loggers.reserve(sinks.size());
      for_each(sinks.begin(), sinks.end(),
               [&loggers, &plant](const PrependedPath<SinkType> &sink) {
                 using TickLog = void(FileLogger::*)(const Tick&);
                 using RawLog = void(FileLogger::*)(const string&);
                 loggers.emplace_back(sink.path);
                 switch(sink.type) {
                   case SinkType::FLAT: {
                     auto handler = bind(static_cast<TickLog>(&FileLogger::log),
                                         &loggers.back(), ph::_1);
                     plant->add_tick_handler(handler);
                     break;
                   }
                   case SinkType::FLAT_RAW: {
                     auto handler = bind(static_cast<RawLog>(&FileLogger::log),
                                         &loggers.back(), ph::_1);
                     plant->add_raw_handler(handler);
                     break;
                   }
                 }
                 cout << "sink " << utils::enum_to_str(sink.type) << " "
                      << sink.path << endl;
               });
    }

    plant->add_tick_handler([](const Tick& tick) {
        if (tick.type == Tick::Type::QUOTE) {
          const Quote& quote = tick.as<Quote>();
          LOG(INFO) << "Q " << quote.received << " " << quote.ex_time << " lag="
                    << (quote.received - quote.ex_time)
                    << " " << quote.total_volume << " @ " << quote.price;

        } else if (tick.type == Tick::Type::TRADE) {
          const Trade& trade = tick.as<Trade>();
        }
        cout << static_cast<int>(tick.type) << endl;
      });
    LOG (INFO) << "starting ticker plant";
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
  return 0;
}
