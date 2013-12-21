#pragma once

#include "ticker_plant.hpp"

#include <leveldb/db.h>
#include <json/value.h>

#include <initializer_list>
#include <memory>
#include <string>
#include <vector>


namespace btc_arb {

class LevelDBLogger {
 public:
  LevelDBLogger(const std::string& path_to_db,
                const TickFilter& filter=&LevelDBLogger::all);

  LevelDBLogger(const LevelDBLogger&) = delete;
  LevelDBLogger& operator=(const LevelDBLogger&) = delete;

  static bool all(const Json::Value&) { return true; }
  void log_tick(const Json::Value& root);
 private:
  TickFilter filter_;
  std::unique_ptr<leveldb::DB> db_;
};

}  // namespace btc_arb
