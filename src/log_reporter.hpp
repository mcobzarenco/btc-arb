#pragma once

#include "ticker_plant.hpp"

#include <json/value.h>

namespace btc_arb {

void report_progress_time(const Tick& tick);
void report_progress_block(const Tick& tick);

}  // namespace btc_arb
