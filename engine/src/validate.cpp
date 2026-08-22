#include <databento/dbn_file_store.hpp>
#include <databento/record.hpp>
#include <databento/symbol_map.hpp>

#include <cassert>
#include <iostream>

#include "dbn_convert.hpp"
#include "event.hpp"
#include "order_book.hpp"

namespace db = databento;
using dbn_convert::make_event_from_msg;

constexpr int INSTRUMENT_ID = 8858; // front-month ES contract on 2021-11-02
const std::string MBP10_PATH =
    "../../data/raw/validation/mbp10/glbx-mdp3-20211102.mbp-10.dbn.zst";
const std::string MBO_PATH =
    "../../data/raw/validation/mbo/glbx-mdp3-20211102.mbo.dbn.zst";

namespace {

void dump(const db::Mbp10Msg &mbp10) {
  for (int i = 0; i < 10; ++i) {
    std::cout << i << ": " << mbp10.levels[i] << '\n';
  }
}

} // namespace

/* the rules determining when a new MBP10 event seem to be a little arcane, and
 * I did not find any documentation about them. trying to actually match the
 * MBP-10 event to the MBO event that triggered it was very hit and miss.
 * instead, we apply the MBO messages to the book (one "atom" at a time; several
 * messages can describe a single event in such a way that the book is invalid
 * when some but not all of them have been applies. this is what ths islast()
 * flag is for.) until our top ten snapshot changes. Once it does, we check that
 * the next mbp10 message matches our top 10. Then we throw away all the
 * consecutive mbp10 messages that have the same top 10, and continue. in this
 * way, we ensure that the sequence of changes to the top 10 that the book
 * experiences is an exact match for what the mbp10 data describes. */
int main() {
  OrderBook book;
  db::DbnFileStore mbp10{MBP10_PATH};
  db::DbnFileStore mbo{MBO_PATH};

  const db::Record *mbo_record = mbo.NextRecord();
  const db::MboMsg *mbo_msg =
      mbo_record ? mbo_record->GetIf<db::MboMsg>() : nullptr;

  auto next_mbp = [&](const db::Mbp10Msg *msg) {
    const db::Record *record = mbp10.NextRecord();
    msg = record ? record->GetIf<db::Mbp10Msg>() : nullptr;
    while (msg != nullptr && msg->hd.instrument_id != INSTRUMENT_ID) {
      record = mbp10.NextRecord();
      msg = record ? record->GetIf<db::Mbp10Msg>() : nullptr;
    }
    return msg;
  };

  // "prev" is the book's top 10 as of the last validated mbp10 record (all
  // undefined/empty before anything has happened).
  databento::BidAskPair prev[10];
  for (int i = 0; i < 10; ++i)
    prev[i] = book.bid_ask_pair(i);

  const db::Mbp10Msg *mbp_msg = next_mbp(nullptr);

  int checked = 0;
  while (mbp_msg != nullptr) {
    // eat every subsequent mbp10 record whose levels already equal `prev`;
    // no new mbo events are needed to validate these
    bool matches_prev = true;
    for (int i = 0; i < 10; ++i) {
      if (mbp_msg->levels[i] != prev[i]) {
        matches_prev = false;
        break;
      }
    }
    if (matches_prev) {
      ++checked;
      mbp_msg = next_mbp(mbp_msg);
      continue;
    }

    // apply mbo events one at a time until the book's top 10 changes and
    // that change matches this mbp10 record's levels
    bool matched = false;
    while (mbo_msg != nullptr) {
      if (mbo_msg->hd.instrument_id == INSTRUMENT_ID) {
        book.send_event(make_event_from_msg(mbo_msg));
        bool changed = false;
        for (int i = 0; i < 10; ++i) {
          if (book.bid_ask_pair(i) != prev[i]) {
            changed = true;
            break;
          }
        }
        if (changed) {
          for (int i = 0; i < 10; ++i)
            prev[i] = book.bid_ask_pair(i);
          matched = true;
          for (int i = 0; i < 10; ++i) {
            if (prev[i] != mbp_msg->levels[i]) {
              matched = false;
              break;
            }
          }
        }
      }
      mbo_record = mbo.NextRecord();
      mbo_msg = mbo_record ? mbo_record->GetIf<db::MboMsg>() : nullptr;
      if (matched)
        break;
    }
    if (!matched) {
      dump(*mbp_msg);
      for (int i = 0; i < 10; ++i) {
        std::cout << book.bid_ask_pair(i) << std::endl;
      }
      assert(false);
    }
    ++checked;
    mbp_msg = next_mbp(mbp_msg);
  }
  std::cout << checked << " mbp-10 snapshot(s) validated\n";
  return 0;
}
