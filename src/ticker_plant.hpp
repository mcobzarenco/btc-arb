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

enum class Currency {
  USD, EUR, GBP, JPY, BTC
};

boost::optional<Currency> from_string(const std::string& cyc);

enum class QuoteType {
  ASK, BID, BEST_ASK, BEST_BID
};

struct Quote {
  const uint64_t received;
  const uint64_t ex_time;
  const double amount;
  const uint64_t amount_int;  // traded amount times 1E8
  const Currency cyc;
  const double price;
  const uint32_t price_int;
  const QuoteType type;

};

struct Trade {
  const uint64_t received;
  const uint64_t ex_time;
  const double amount;
  const uint64_t amount_int;  // traded amount times 1E8
  const Currency cyc;
  const double price;
  const uint32_t price_int;
  const QuoteType trade_type;
};

class Tick {
 public:
  enum class Type { EMPTY, QUOTE, TRADE };

  template<typename T>
  inline const T& as() {
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

template<> class Tick::ContentInd<Quote> {
  static constexpr Type CONTENT_TYPE = Type::QUOTE;
};

template<> class Tick::ContentInd<Trade> {
  static constexpr Type CONTENT_TYPE = Type::TRADE;
};


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
  boost::optional<Tick> tick = Parser::parse(msg->get_payload());
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
    boost::optional<Tick> tick = Parser::parse(it->value().ToString());
    if (tick) {
        call_handlers(*tick);
    } else {
        // LOG(WARNING) << "Could not parse tick:" << it->value().ToString();
    }
    // LOG(WARNING) << "Could not parse tick (" << reader.getFormattedErrorMessages()
    //              << ") raw tick=" << it->value().ToString();
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
