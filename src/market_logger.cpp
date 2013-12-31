#include "market_logger.hpp"

#include <glog/logging.h>
#include <json/writer.h>

#include <iostream>


namespace btc_arb {

using namespace std;


LevelDBLogger::LevelDBLogger(const string& path_to_db) {
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

FileLogger::FileLogger(const std::string& path_to_file) {
  file_.open(path_to_file, ios::out | ios::app);
}




}  // namespace btc_arb
