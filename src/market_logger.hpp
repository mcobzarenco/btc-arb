#pragma once

#include "ticker_plant.hpp"

#include <leveldb/db.h>
#include <json/value.h>

#include <chrono>
#include <initializer_list>
#include <fstream>
#include <memory>
#include <string>
#include <vector>


namespace btc_arb {

class LevelDBLogger {
 public:
  LevelDBLogger(const std::string& path_to_db);

  LevelDBLogger(const LevelDBLogger&) = delete;

  inline void log(const std::string& key, const std::string& value);
 private:
  TickFilter filter_;
  std::unique_ptr<leveldb::DB> db_;
};

void LevelDBLogger::log(const std::string& key, const std::string& value) {
  CHECK (db_) << "LevelDB database not open";
  leveldb::WriteOptions opts;
  db_->Put(opts, key, value);
}

// void to_leveldb_key_timestamp(LevelDBLogger& logger, const std::string& msg) {
//   std::string now = std::to_string(
//       std::chrono::system_clock::now().time_since_epoch().count());
//   logger.log(now, msg);
// }

class FileLogger {
 public:
  FileLogger(const std::string& path_to_file);

  FileLogger(const FileLogger&) = delete;

  inline void log(const std::string& tick);
 private:
  std::ofstream file_;
};

void FileLogger::log(const std::string& tick) {
  CHECK (file_.is_open()) << "file not open";
  file_ << tick;
}


}  // namespace btc_arb
