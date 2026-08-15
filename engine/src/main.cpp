#include "csv.h"
#include "event.hpp"
#include "order_book.hpp"

#include <cmath>
#include <cstdio>
#include <ctime>
#include <iostream>

namespace {

uint64_t parse_ts_ns(const std::string &s) {
  int y, mo, d, h, mi, se, ns = 0;
  std::sscanf(s.c_str(), "%d-%d-%dT%d:%d:%d.%dZ", &y, &mo, &d, &h, &mi, &se,
              &ns);
  std::tm tm{};
  tm.tm_year = y - 1900;
  tm.tm_mon = mo - 1;
  tm.tm_mday = d;
  tm.tm_hour = h;
  tm.tm_min = mi;
  tm.tm_sec = se;
  return static_cast<uint64_t>(timegm(&tm)) * 1'000'000'000ULL +
         static_cast<uint64_t>(ns);
}

Action parse_action(char c) {
  switch (c) {
  case 'A':
    return Action::Add;
  case 'C':
    return Action::Cancel;
  case 'M':
    return Action::Modify;
  case 'R':
    return Action::Clear;
  case 'T':
    return Action::Trade;
  case 'F':
    return Action::Fill;
  default:
    return Action::None;
  }
}

Side parse_side(char c) {
  switch (c) {
  case 'B':
    return Side::Bid;
  case 'A':
    return Side::Ask;
  default:
    return Side::None;
  }
}

} // namespace

int main() {
  OrderBook book;
  io::CSVReader<15> in("../../data/raw/TenMinutesESdata.csv");
  in.read_header(io::ignore_extra_column, "ts_recv", "ts_event", "rtype",
                 "publisher_id", "instrument_id", "action", "side", "price",
                 "size", "channel_id", "order_id", "flags", "ts_in_delta",
                 "sequence", "symbol");

  std::string ts_recv_str, ts_event_str, symbol;
  uint8_t rtype;
  uint16_t publisher_id;
  uint32_t instrument_id;
  char action_c, side_c;
  double price_d;
  uint32_t size;
  uint16_t channel_id;
  uint64_t order_id;
  uint8_t flags;
  int32_t ts_in_delta;
  uint64_t sequence;

  std::vector<MarketEvent> events;

  auto start_read = std::chrono::high_resolution_clock::now();
  while (in.read_row(ts_recv_str, ts_event_str, rtype, publisher_id,
                     instrument_id, action_c, side_c, price_d, size, channel_id,
                     order_id, flags, ts_in_delta, sequence, symbol)) {
    MarketEvent e(parse_ts_ns(ts_recv_str), size, parse_ts_ns(ts_event_str),
                  channel_id, rtype, order_id, publisher_id, flags,
                  instrument_id, ts_in_delta, parse_action(action_c), sequence,
                  parse_side(side_c), symbol,
                  static_cast<int64_t>(std::llround(price_d * 1e9)));
    events.push_back(e);
  }
  auto end_read = std::chrono::high_resolution_clock::now();
  auto start_process = std::chrono::high_resolution_clock::now();
  for (const auto &e : events) {
    book.send_event(e);
  }
  auto end_process = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double, std::milli> read_duration =
      end_read - start_read;
  std::cout << "reading events took: " << read_duration.count() << " ms\n";
  std::chrono::duration<double, std::milli> process_duration =
      end_process - start_process;
  std::cout << "processing events took: " << process_duration.count()
            << " ms\n";
  return 0;

  // 3. Calculate the duration by subtracting start from end

  // 4. Output the result
}
