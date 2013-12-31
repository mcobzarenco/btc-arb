#include "log_reporter.hpp"

#include <glog/logging.h>

#include <chrono>
#include <iomanip>
#include <string>


namespace btc_arb {
namespace mtgox {

using namespace std;

namespace {
constexpr double REPORT_FREQ = 1.0;
constexpr uint32_t REPORT_COUNT = 10000;

constexpr char CHANNEL_TRADES[] = "dbf1dee9-4f2e-4a08-8cb7-748919a71b21";
constexpr char CHANNEL_TICKER[] = "d5f06780-30a8-4a48-a2f8-7ed181b4a13f";
constexpr char CHANNEL_DEPTH[] = "24e67e0d-1cad-4cc0-9e7a-f8523ef460fe";
}

class TickCounter {
 public:
  void operator() (const Json::Value& root) {
    ++count_;
    string tick_type = root.get("channel", "").asString();
    if (tick_type == CHANNEL_TRADES) {
      ++trades_;
    } else if (tick_type == CHANNEL_DEPTH ||
               tick_type == CHANNEL_TICKER) {
      ++book_updates_;
    } else {
      ++unknowns_;
    }
  }

  int get_count() const { return count_; }
  int get_book_updates() const { return book_updates_; }
  int get_trades() const { return trades_; }
  int get_unknowns() const { return unknowns_; }

  void reset() {
    count_ = 0;
    book_updates_ = 0;
    trades_ = 0;
    unknowns_ = 0;
  }

 private:
  int count_ = 0;
  int book_updates_ = 0;
  int trades_ = 0;
  int unknowns_ = 0;
};

void report_progress_time(const Json::Value& root) {
  static TickCounter counter;
  static auto last = std::chrono::system_clock::now();

  counter(root);
  auto now = std::chrono::system_clock::now();
  chrono::duration<double> elapsed_seconds = now - last;
  if (elapsed_seconds.count() >= REPORT_FREQ) {
    LOG(INFO) << "[" << setprecision(4)
              << elapsed_seconds.count() << setfill('0') << setw(4) << "s] "
              << setfill(' ') << setw(5) << counter.get_trades() << "t / "
              << setw(5) << counter.get_book_updates() << "bu / "
              << setw(5) << counter.get_unknowns() << "un";
    last = now;
    counter.reset();
  }
}

void report_progress_block(const Json::Value& root) {
  static TickCounter counter;
  static auto last = std::chrono::system_clock::now();
  counter(root);
  if (counter.get_count() % REPORT_COUNT == 0) {
    auto now = std::chrono::system_clock::now();
    chrono::duration<double> elapsed_seconds = now - last;
    LOG(INFO) << "[" << setprecision(4)
              << elapsed_seconds.count() << setfill('0') << setw(4) << "s] "
              << setfill(' ') << setw(5) << counter.get_trades() << "t / "
              << setw(5) << counter.get_book_updates() << "bu / "
              << setw(5) << counter.get_unknowns() << "un";
    last = now;
    counter.reset();
  }
}

}}  // namespce btc_arb::mtgox
