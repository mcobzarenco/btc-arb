#pragma once

#include "enum_utils.hpp"

#include <boost/optional.hpp>
#include <glog/logging.h>
#include <json/value.h>
#include <json/reader.h>
#include <leveldb/db.h>
#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>

#include <fstream>
#include <functional>
#include <string>
#include <sstream>
#include <cstdint>
#include <vector>


namespace btc_arb {

constexpr int VOLUME_MULTIPLIER = 100000000;  // 1E8

enum class Currency {
  USD, EUR, GBP, JPY, BTC
};

template<> char const* utils::EnumStrings<Currency>::names[] = {
  "usd", "eur", "gbp", "jpy", "btc"};


boost::optional<const Currency> ccy_from_string(const std::string& cyc);

struct Quote {
  enum class Type {
    ASK_UPDATE, BID_UPDATE,
  };

  uint64_t received;
  uint64_t ex_time;
  Type type;
  double delta_volume;
  int64_t delta_volume_int;
  double total_volume;
  int64_t total_volume_int;  // volume times VOLUME_MULTIPLIER (1E8)
  Currency cyc;
  double price;
  int32_t price_int;
};

struct Trade {
  enum class Type {
    ASK, BID
  };

  uint64_t received;
  uint64_t ex_time;
  Type type;
  double amount;
  int64_t amount_int;  // amount times 1E8
  Currency cyc;
  double price;
  int32_t price_int;
};

class Tick {
 public:
  enum class Type { EMPTY, QUOTE, TRADE };

  template<typename T>
  inline const T& as() const {
    CHECK (ContentInd<T>::CONTENT_TYPE == type);
    return *reinterpret_cast<const T*>(&tickc_);
  }

  Tick() : type(Type::EMPTY) {}
  Tick(const Quote& quote) : type(Type::QUOTE), tickc_{quote} {}
  Tick(const Trade& trade) : type(Type::TRADE), tickc_{trade} {}
  Tick(const Tick&) = default;
  Tick& operator=(const Tick&) = default;

 private:
  template<typename T>  class ContentInd {};
  union TickContent {
    TickContent() {}
    TickContent(const Quote& quote_) : quote(quote_) {}
    TickContent(const Trade& trade_) : trade(trade_) {}
    TickContent(const TickContent&) = default;
    TickContent& operator=(const TickContent&) = default;

    Quote quote;
    Trade trade;
  };

 public:
  Type type;
 private:
  TickContent tickc_;
};

template<> struct Tick::ContentInd<Quote> {
  static constexpr Type CONTENT_TYPE = Type::QUOTE;
};
// constexpr Tick::Type Tick::ContentInd<Quote>::CONTENT_TYPE;

template<> struct Tick::ContentInd<Trade> {
  static constexpr Type CONTENT_TYPE = Type::TRADE;
};
// constexpr Tick::Type Tick::ContentInd<Trade>::CONTENT_TYPE;


using TickHandler = std::function<void(const Tick&)>;
using RawHandler = std::function<void(const std::string&)>;

class TickerPlant {
 public:
  void add_tick_handler(TickHandler&& handler);
  void add_raw_handler(RawHandler&& handler);
  virtual bool run() = 0;
 protected:
  inline void call_handlers(const Tick& tick);
  inline void call_raw_handlers(const std::string& msg);

  std::vector<TickHandler> handlers_;
  std::vector<RawHandler> raw_handlers_;
};

void TickerPlant::call_handlers(const Tick& tick) {
  for(auto& handler : handlers_) {
    handler(tick);
  }
}

void TickerPlant::call_raw_handlers(const std::string& msg) {
  for(auto& handler : raw_handlers_) {
    handler(msg);
  }
}

struct ParsedTick {
  Tick tick;
  std::string raw;
};

class FlatParser {
 protected:
  inline boost::optional<const ParsedTick> parse(std::istream& stream) {
    Tick tick;
    if (stream.read(reinterpret_cast<char*>(&tick), sizeof(Tick))) {
      return boost::optional<const ParsedTick>(ParsedTick{tick, ""});
    }
    return boost::optional<const ParsedTick>{};
  }
};

template<typename Parser>
class WebSocketTickerPlant : public TickerPlant, Parser {
 public:
  using ws_client = websocketpp::client<websocketpp::config::asio_client>;
  using message_ptr = websocketpp::config::asio_client::message_type::ptr;

  WebSocketTickerPlant(const std::string& uri_);

  WebSocketTickerPlant(const WebSocketTickerPlant&) = delete;

  virtual bool run() override;
 private:
  inline void dispatcher(websocketpp::connection_hdl hdl, message_ptr msg);

  const std::string uri_;
  ws_client client_;
};

template<typename Parser>
WebSocketTickerPlant<Parser>::WebSocketTickerPlant(const std::string& uri)
    : uri_(uri) {
  client_.init_asio();
}

template<typename Parser>
bool WebSocketTickerPlant<Parser>::run() {
  client_.set_message_handler(
      bind(&WebSocketTickerPlant::dispatcher, this, _1, _2));
  websocketpp::lib::error_code ec;
  ws_client::connection_ptr conn = client_.get_connection(uri_, ec);
  conn->replace_header("Origin", uri_);
  client_.connect(conn);
  client_.run();
  return true;
}

template<typename Parser>
void WebSocketTickerPlant<Parser>::dispatcher(
    websocketpp::connection_hdl hdl, message_ptr msg) {
  std::stringstream stream{msg->get_payload()};
  boost::optional<const ParsedTick> parsed = Parser::parse(stream);
  if (parsed) {
    call_handlers((*parsed).tick);
    call_raw_handlers((*parsed).raw);
  } else {
    LOG(INFO) << "un-handled event";
  }
}

template<typename Parser>
class FileTickerPlant : public TickerPlant {
 public:
  FileTickerPlant(const std::string& path_to_file);

  FileTickerPlant(const FileTickerPlant&) = delete;

  virtual bool run() override;
 private:
  std::ifstream file_;
};

class FileLogger {
 public:
  FileLogger(const std::string& path_to_file);

  FileLogger(const FileLogger&) = delete;
  FileLogger(FileLogger&&) = default;

  inline void log(const char* tick, size_t size);
  inline void log(const std::string& msg);
  inline void log(const Tick& tick);
 private:
  std::unique_ptr<std::ofstream> file_;
};

FileLogger::FileLogger(const std::string& path_to_file) {
  file_.reset(new std::ofstream());
  file_->open(path_to_file, std::ios::out | std::ios::app);
}

void FileLogger::log(const char* tick, size_t size) {
  CHECK (file_->is_open()) << "file not open";
  file_->write(tick, size);
}

void FileLogger::log(const std::string& msg) {
  CHECK (file_->is_open()) << "file not open";
  (*file_) << msg;
}

void FileLogger::log(const Tick& tick) {
  log(reinterpret_cast<const char *>(&tick), sizeof(Tick));
}

}  // namespace btc_arb
