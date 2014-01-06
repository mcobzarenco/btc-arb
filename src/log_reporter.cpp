#include "log_reporter.hpp"

#include <glog/logging.h>

#include <chrono>
#include <iomanip>
#include <string>


namespace btc_arb {

using namespace std;

namespace {
constexpr double REPORT_FREQ = 1.0;
constexpr uint32_t REPORT_COUNT = 10000;
}

class TickCounter {
 public:
  void operator() (const Tick& tick) {
    ++count_;
    if (tick.type == Tick::Type::TRADE) {
      ++trades_;
    } else if (tick.type == Tick::Type::QUOTE) {
      ++quotes_;
    }
  }

  int get_count() const { return count_; }
  int get_trades() const { return trades_; }
  int get_quotes() const { return quotes_; }

  void reset() {
    count_ = 0;
    trades_ = 0;
    quotes_ = 0;
  }

 private:
  int count_ = 0;
  int trades_ = 0;
  int quotes_ = 0;
};

void report_progress_time(const Tick& tick) {
  static TickCounter counter;
  static auto last = std::chrono::system_clock::now();

  counter(tick);
  auto now = std::chrono::system_clock::now();
  chrono::duration<double> elapsed_seconds = now - last;
  if (elapsed_seconds.count() >= REPORT_FREQ) {
    LOG(INFO) << "[" << setprecision(4)
              << elapsed_seconds.count() << setfill('0') << setw(4) << "s] "
              << setfill(' ') << setw(5) << counter.get_trades() << "trades / "
              << setw(5) << counter.get_quotes() << " quotes";
    last = now;
    counter.reset();
  }
}

void report_progress_block(const Tick& tick) {
  static TickCounter counter;
  static auto last = std::chrono::system_clock::now();
  counter(tick);
  if (counter.get_count() % REPORT_COUNT == 0) {
    auto now = std::chrono::system_clock::now();
    chrono::duration<double> elapsed_seconds = now - last;
    LOG(INFO) << "[" << setprecision(4)
              << elapsed_seconds.count() << setfill('0') << setw(4) << "s] "
              << setfill(' ') << setw(5) << counter.get_trades() << "trades / "
              << setw(5) << counter.get_quotes() << "quotes";
    last = now;
    counter.reset();
  }
}

}  // namespce btc_arb
