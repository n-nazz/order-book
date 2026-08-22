#include <databento/dbn_file_store.hpp>
#include <databento/record.hpp>

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <vector>

#include "dbn_convert.hpp"
#include "event.hpp"
#include "order_book.hpp"

namespace db = databento;

namespace {

constexpr int INSTRUMENT_ID = 8858;
const std::string MBO_PATH =
    "../../data/raw/validation/mbo/glbx-mdp3-20211102.mbo.dbn.zst";

} // namespace

int main(int argc, char **argv) {
  const size_t limit = argc > 1 ? std::strtoull(argv[1], nullptr, 10)
                                : std::numeric_limits<size_t>::max();

  db::DbnFileStore mbo{MBO_PATH};
  std::vector<MarketEvent> events;
  auto start_read = std::chrono::high_resolution_clock::now();
  while (events.size() < limit) {
    const db::Record *record = mbo.NextRecord();
    if (!record)
      break;
    const db::MboMsg *msg = record->GetIf<db::MboMsg>();
    if (!msg || msg->hd.instrument_id != INSTRUMENT_ID)
      continue;
    events.push_back(dbn_convert::make_event_from_msg(msg));
  }
  auto end_read = std::chrono::high_resolution_clock::now();

  OrderBook book;
  auto start_process = std::chrono::high_resolution_clock::now();
  for (const auto &e : events) {
    book.send_event(e);
  }
  auto end_process = std::chrono::high_resolution_clock::now();

  std::chrono::duration<double, std::milli> read_ms = end_read - start_read;
  std::chrono::duration<double, std::milli> process_ms =
      end_process - start_process;
  double events_per_sec = events.size() / (process_ms.count() / 1000.0);

  std::cout << events.size() << " events read in " << read_ms.count()
            << " ms\n";
  std::cout << events.size() << " events processed in " << process_ms.count()
            << " ms (" << events_per_sec << " events/sec)\n";
  return 0;
}
