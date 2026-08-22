#pragma once

#include <cassert>

#include <databento/record.hpp>

#include "event.hpp"

namespace dbn_convert {

inline Action to_action(databento::Action action) {
  switch (action) {
  case databento::Action::Add:
    return Action::Add;
  case databento::Action::Cancel:
    return Action::Cancel;
  case databento::Action::Modify:
    return Action::Modify;
  case databento::Action::Clear:
    return Action::Clear;
  case databento::Action::Trade:
    return Action::Trade;
  case databento::Action::Fill:
    return Action::Fill;
  case databento::Action::None:
    return Action::None;
  }
  assert(false);
}

inline Side to_side(databento::Side side) {
  switch (side) {
  case databento::Side::Ask:
    return Side::Ask;
  case databento::Side::Bid:
    return Side::Bid;
  case databento::Side::None:
    return Side::None;
  }
  assert(false);
}

inline MarketEvent make_event_from_msg(const databento::MboMsg *msg) {
  return MarketEvent(msg->ts_recv.time_since_epoch().count(), msg->size,
                     msg->hd.ts_event.time_since_epoch().count(),
                     msg->channel_id, static_cast<uint8_t>(msg->hd.rtype),
                     msg->order_id, msg->hd.publisher_id, msg->flags.Raw(),
                     msg->hd.instrument_id, msg->ts_in_delta.count(),
                     to_action(msg->action), msg->sequence, to_side(msg->side),
                     /*symbol=*/"", msg->price);
}

} // namespace dbn_convert
