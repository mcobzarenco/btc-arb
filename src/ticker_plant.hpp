#pragma once

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
#include <cstdint>
#include <vector>


namespace btc_arb {

constexpr int VOLUME_MULTIPLIER = 100000000;  // 1E8

enum class Currency {
  USD, EUR, GBP, JPY, BTC
};

boost::optional<const Currency> ccy_from_string(const std::string& cyc);

struct Quote {
  enum class Type {
    ASK_UPDATE, BID_UPDATE,
  };

  const uint64_t received;
  const uint64_t ex_time;
  const Type type;
  const double delta_volume;
  const int64_t delta_volume_int;
  const double total_volume;
  const int64_t total_volume_int;  // volume times VOLUME_MULTIPLIER (1E8)
  const Currency cyc;
  const double price;
  const int32_t price_int;
};

struct Trade {
  enum class Type {
    ASK, BID
  };

  const uint64_t received;
  const uint64_t ex_time;
  const Type type;
  const double amount;
  const int64_t amount_int;  // amount times 1E8
  const Currency cyc;
  const double price;
  const int32_t price_int;
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

 private:
  template<typename T>  class ContentInd {};
  union TickContent {
    TickContent() {}
    TickContent(const Quote& quote_) : quote(quote_) {}
    TickContent(const Trade& trade_) : trade(trade_) {}
    Quote quote;
    Trade trade;
  };

 public:
  const Type type;
 private:
  const TickContent tickc_;
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
using TickFilter = std::function<bool(const Tick&)>;

class TickerPlant {
 public:
  void add_tick_handler(TickHandler&& handler);
  virtual bool run() = 0;
 protected:
  inline void call_handlers(const Tick& tick);

  std::vector<TickHandler> handlers_;
};

void TickerPlant::call_handlers(const Tick& tick) {
    for(auto& handler : handlers_) {
        handler(tick);
    }
}

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
  boost::optional<const Tick> tick = Parser::parse(msg->get_payload());
  if (tick) {
    call_handlers(*tick);
  } else {
    LOG(INFO) << "un-handled event";
  }
}

class FlatParser {
 protected:
  inline boost::optional<const Tick> parse(const std::string& msg) {
    return boost::optional<const Tick>(*reinterpret_cast<const Tick*>(msg.c_str()));
  }
};

template<typename Parser>
class LdbTickerPlant : public TickerPlant, Parser {
 public:
  LdbTickerPlant(const std::string& path_to_db);

  LdbTickerPlant(const LdbTickerPlant&) = delete;

  virtual bool run() override;
 private:
  std::string path_to_db_;
  std::vector<TickHandler> handlers_;
};

template<typename Parser>
LdbTickerPlant<Parser>::LdbTickerPlant(const std::string& path_to_db)
    : path_to_db_(path_to_db) {
}

template<typename Parser>
bool LdbTickerPlant<Parser>::run() {
  std::unique_ptr<leveldb::DB> db;
  {
    leveldb::DB *ptr_db{nullptr};
    leveldb::Options options;
    options.create_if_missing = false;
    leveldb::Status status =
        leveldb::DB::Open(options, path_to_db_, &ptr_db);
    if (!status.ok()) {
      LOG(FATAL) << status.ToString();
      return false;
    }
    db.reset(ptr_db);
  }
  CHECK (db) << "LevelDB database not open";

  std::unique_ptr<leveldb::Iterator> it(db->NewIterator(leveldb::ReadOptions()));
  for (it->SeekToFirst(); it->Valid(); it->Next()) {
    boost::optional<const Tick> tick = Parser::parse(it->value().ToString());
    if (tick) {
      call_handlers(*tick);
    }
  }
  return it->status().ok();
}

class FlatFileTickerPlant : public TickerPlant {
 public:
  FlatFileTickerPlant(const std::string& path_to_file);

  FlatFileTickerPlant(const FlatFileTickerPlant&) = delete;

  virtual bool run() override;
 private:
  std::ifstream file_;
};

}  // namespace btc_arb
