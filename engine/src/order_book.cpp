#include "order_book.hpp"
#include "event.hpp"
#include "record.hpp"
#include <cassert>
#include <cstdint>
#include <iostream>

using std::endl, std::cout;

OrderBook::OrderBook(int64_t minPrice, int64_t maxPrice, size_t tick_size)
    : bids(false, minPrice, maxPrice, tick_size),
      asks(true, minPrice, maxPrice, tick_size) {}

void OrderBook::debug_print() {
  cout << " === BIDS === ";
  for (size_t i = 0; i < bids.size(); ++i) {
    int64_t price = bids.nth_best_price(i);
    cout << "price : " << price << endl;
    cout << bids[price];
  }
  cout << " === END BIDS ===" << endl;
  cout << " === ASKS === ";
  for (size_t i = 0; i < asks.size(); ++i) {
    int64_t price = asks.nth_best_price(i);
    cout << "price : " << price << endl;
    cout << asks[price];
  }
  cout << " === END ASKS ===";
}

void OrderBook::send_event(const MarketEvent &e) {
  switch (e.action()) {
  case Action::Add:
    add_event(e);
    break;
  case Action::Modify:
    modify_event(e);
    break;
  case Action::Clear:
    clear();
    break;
  case Action::Trade:
    trade_event(e);
    break;
  case Action::Fill:
    fill_event(e);
    break;
  case Action::None:
    none_event(e);
    break;
  case Action::Cancel:
    cancel_event(e);
    break;
  }
}

void OrderBook::add_event(const MarketEvent &e) {
  assert(e.side() != Side::None);
  assert(!orders_by_id.contains(e.order_id()));
  BidsOrAsks &side = (e.side() == Side::Ask) ? asks : bids;
  orders_by_id[e.order_id()] = side.insert_order(e);
}

void OrderBook::modify_event(const MarketEvent &e) {
  assert(orders_by_id.contains(e.order_id()));
  assert(e.side() != Side::None);
  BidsOrAsks &side = (e.side() == Side::Ask ? asks : bids);
  auto oldOrder = orders_by_id[e.order_id()];
  if (e.price() == oldOrder->price) {
    side.modify_order(oldOrder->price, oldOrder, e.size());
  } else {
    side.move_order(oldOrder->price, e.price(), oldOrder, e.size(),
                     e.sequence());
  }
}

void OrderBook::clear() {
  bids.clear();
  asks.clear();
  orders_by_id.clear();
}

void OrderBook::trade_event(const MarketEvent &e) {
  // TODO this doesn't affect the book but should be responded to in some way
}

void OrderBook::fill_event(const MarketEvent &e) {
  // TODO this doesn't affect the book but should be responded to in some way
}

void OrderBook::none_event(const MarketEvent &e) {
  // TODO check flags or something
}

void OrderBook::cancel_event(const MarketEvent &e) {
  assert(e.side() != Side::None);
  assert(orders_by_id.contains(e.order_id()));
  BidsOrAsks &side = (e.side() == Side::Ask) ? asks : bids;
  auto it = orders_by_id[e.order_id()];
  assert(it->quantity >= e.size());
  if (side.cancel_order(e.price(), it, e.size())) {
    orders_by_id.erase(e.order_id());
  }
}

databento::BidAskPair OrderBook::bid_ask_pair(size_t level) {
  int bid_sz, bid_ct, ask_sz, ask_ct;
  int64_t ask_px, bid_px;
  if (bids.size() <= level) {
    bid_px = databento::kUndefPrice;
    bid_sz = 0;
    bid_ct = 0;
  } else {
    bid_px = bids.nth_best_price(level);
    PriceLevel &pricelevel = bids[bid_px];
    bid_sz = pricelevel.total_volume;
    bid_ct = pricelevel.queue.size();
  }
  if (asks.size() <= level) {
    ask_px = databento::kUndefPrice;
    ask_sz = 0;
    ask_ct = 0;
  } else {
    ask_px = asks.nth_best_price(level);
    PriceLevel &pricelevel = asks[ask_px];
    ask_sz = pricelevel.total_volume;
    ask_ct = pricelevel.queue.size();
  }
  return databento::BidAskPair(bid_px, ask_px, bid_sz, ask_sz, bid_ct, ask_ct);
}
