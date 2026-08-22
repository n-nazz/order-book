#include "order_book.hpp"
#include "constants.hpp"
#include "event.hpp"
#include "record.hpp"
#include <cassert>
#include <cstdint>
#include <iostream>
#include <iterator>
#include <unordered_map>

using std::endl, std::cout;

Order::Order(const MarketEvent &e) {
  quantity = e.size();
  sequence = e.sequence();
  price = e.price();
  id = e.order_id();
}

Order::Order(uint64_t a_quantity, uint64_t a_sequence, int64_t a_price,
             uint64_t a_id) {
  quantity = a_quantity;
  sequence = a_sequence;
  price = a_price;
  id = a_id;
}

PriceLevel::PriceLevel(uint64_t volume)
    : total_volume{volume}, queue{std::list<Order>()} {}

PriceLevel::PriceLevel(const MarketEvent &e)
    : total_volume(e.size()), queue{std::list<Order>{Order(e)}} {}

PriceLevel::PriceLevel() : total_volume{0}, queue{std::list<Order>()} {}

void OrderBook::debug_print() {
  cout << " === BIDS === ";
  for (const auto &[price, pricelevel] : bids) {
    cout << "price : " << price << endl;
    cout << pricelevel;
  }
  cout << " === END BIDS ===" << endl;
  cout << " === ASKS === ";
  for (const auto &[price, pricelevel] : asks) {
    cout << "price : " << price << endl;
    cout << pricelevel;
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
  std::map<int64_t, PriceLevel> &side = (e.side() == Side::Ask) ? asks : bids;
  if (side.contains(e.price())) {
    PriceLevel &pricelevel = side[e.price()];
    pricelevel.total_volume += e.size();
    auto it = pricelevel.queue.end();
    while (it != pricelevel.queue.begin() and
           std::prev(it)->sequence > e.sequence())
      --it;
    pricelevel.queue.insert(it, Order(e));
    orders_by_id[e.order_id()] = std::prev(it);
  } else {
    // TODO PriceLevel needs a move ctor? make sure the below makes sense
    side[e.price()] = PriceLevel(e);
    orders_by_id[e.order_id()] = side[e.price()].queue.begin();
  }
}

void OrderBook::modify_event(const MarketEvent &e) {
  assert(orders_by_id.contains(e.order_id()));
  assert(e.side() != Side::None);
  auto &side = (e.side() == Side::Ask ? asks : bids);
  auto oldOrder = orders_by_id[e.order_id()];
  if (e.price() == oldOrder->price) {
    side[oldOrder->price].total_volume -= (oldOrder->quantity - e.size());
    oldOrder->quantity = e.size();
  } else {
    int64_t old_price = oldOrder->price;
    // make sure there's a PriceLevel to move oldOrder to
    if (!side.contains(e.price())) {
      side[e.price()] = PriceLevel(0);
    }
    side[e.price()].total_volume += e.size();
    side[old_price].total_volume -= oldOrder->quantity;
    auto insert_pos = side[e.price()].queue.end();
    while (insert_pos != side[e.price()].queue.begin() and
           std::prev(insert_pos)->sequence > e.sequence())
      --insert_pos;
    side[e.price()].queue.splice(insert_pos, side[old_price].queue,
                                 oldOrder);
    if (side[old_price].total_volume == 0) {
      side.erase(old_price);
    }
    oldOrder->price = e.price();
    oldOrder->quantity = e.size();
    oldOrder->sequence = e.sequence();
  }
}

void OrderBook::clear() {
  bids.clear();
  asks.clear();
  orders_by_id.clear(); //
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
  std::map<int64_t, PriceLevel> &side = (e.side() == Side::Ask) ? asks : bids;
  auto it = orders_by_id[e.order_id()];
  assert(it->quantity >= e.size());
  it->quantity -= e.size(); // partial cancellation
  side[e.price()].total_volume -= e.size();
  if (it->quantity == 0) {
    side[e.price()].queue.erase(it);
    orders_by_id.erase(e.order_id());
  }
  if (side[e.price()].total_volume == 0) {
    side.erase(e.price());
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
    // bids map is sorted ascending; best bid (highest price) is at the end
    auto it = std::next(bids.rbegin(), level);
    bid_px = it->first;
    bid_sz = it->second.total_volume;
    bid_ct = it->second.queue.size();
  }
  if (asks.size() <= level) {
    ask_px = databento::kUndefPrice;
    ask_sz = 0;
    ask_ct = 0;
  } else {
    auto it = std::next(asks.begin(), level);
    ask_px = it->first;
    ask_sz = it->second.total_volume;
    ask_ct = it->second.queue.size();
  }
  return databento::BidAskPair(bid_px, ask_px, bid_sz, ask_sz, bid_ct, ask_ct);
}
