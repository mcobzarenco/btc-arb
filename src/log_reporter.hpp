#pragma once

#include <json/value.h>

namespace btc_arb {
namespace mtgox {

void report_progress_time(const Json::Value& root);
void report_progress_block(const Json::Value& root);

}}  // namespace btc_arb::mtgox
