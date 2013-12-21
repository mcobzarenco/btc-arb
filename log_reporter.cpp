#include "log_reporter.hpp"

#include <glog/logging.h>

#include <chrono>
#include <iomanip>
#include <string>


namespace btc_arb {

constexpr double REPORT_FREQ = 5.0;

constexpr char MTGOX_CHANNEL_TRADES[] = "dbf1dee9-4f2e-4a08-8cb7-748919a71b21";
constexpr char MTGOX_CHANNEL_TICKER[] = "d5f06780-30a8-4a48-a2f8-7ed181b4a13f";
constexpr char MTGOX_CHANNEL_DEPTH[] = "24e67e0d-1cad-4cc0-9e7a-f8523ef460fe";

using namespace std;

void report_progress_mtgox(const Json::Value& root) {
    static int n_book_updates = 0, n_trades = 0, n_unknown = 0;
    static auto last = std::chrono::system_clock::now();

    string tick_type = root.get("channel", "").asString();

    if(tick_type == MTGOX_CHANNEL_TRADES) {
        ++n_trades;
    } else if (tick_type == MTGOX_CHANNEL_DEPTH ||
               tick_type == MTGOX_CHANNEL_TICKER) {
        ++n_book_updates;
    } else {
        ++n_unknown;
    }

    auto now = std::chrono::system_clock::now();
    chrono::duration<double> elapsed_seconds = now - last;
    if (elapsed_seconds.count() >= REPORT_FREQ) {
        LOG(INFO) << "[" << setprecision(4)
                  << elapsed_seconds.count() << setfill('0') << setw(4) << "s] "
                  << setfill(' ') << setw(5) << n_trades << " trades / "
                  << setw(5) << n_book_updates << " book updates / "
                  << setw(5) << n_unknown << " unknown ticks";
        last = now;
        n_book_updates = 0;
        n_trades = 0;
        n_unknown = 0;
    }
}

}  // namespce btc_arb
