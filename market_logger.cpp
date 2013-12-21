#include "market_logger.hpp"

#include <glog/logging.h>
#include <json/writer.h>

#include <chrono>
#include <iostream>


namespace btc_arb {

using namespace std;


LevelDBLogger::LevelDBLogger(
    const string& path_to_db, const TickFilter& filter)
        : filter_(filter) {
    leveldb::DB *ptr_db{nullptr};
    leveldb::Options options;
    options.create_if_missing = true;
    leveldb::Status status =
        leveldb::DB::Open(options, path_to_db, &ptr_db);
    if (!status.ok()) {
      LOG(FATAL) << status.ToString();
      return;
    }
    db_.reset(ptr_db);
}

void LevelDBLogger::log_tick(const Json::Value& root) {
  CHECK (db_) << "No open LevelDB database";
  Json::Value copy = root;
  leveldb::WriteOptions opts;
  stringstream key;
  Json::FastWriter writer;
  auto now = to_string(chrono::system_clock::now().time_since_epoch().count());
  copy["_received"] = Json::Value(now);
  db_->Put(opts, now, writer.write(copy));
}

}  // namespace btc_arb
