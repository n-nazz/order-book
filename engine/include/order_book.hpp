#pragma once

#include "bids_or_asks.hpp"
#include "event.hpp"
#include "record.hpp"
#include <cstdint>
#include <list>
#include <unordered_map>

class OrderBook {
public:
  OrderBook(int64_t minPrice, int64_t maxPrice, size_t tick_size);
  void send_event(const MarketEvent &e);
  void debug_print();
  // requests for data about the book to follow
  databento::BidAskPair bid_ask_pair(size_t level);

private:
  BidsOrAsks bids, asks;
  std::unordered_map<uint64_t, std::list<Order>::iterator> orders_by_id;
  void add_event(const MarketEvent &e);
  void modify_event(const MarketEvent &e);
  void clear();
  void trade_event(const MarketEvent &e);
  void fill_event(const MarketEvent &e);
  void none_event(const MarketEvent &e);
  void cancel_event(const MarketEvent &e);
};
