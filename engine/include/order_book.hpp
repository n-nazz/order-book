#pragma once

#include "event.hpp"
#include <cstdint>
#include <list>
#include <map>
#include <ostream>
#include <unordered_map>

struct Order {
  uint64_t quantity;
  uint64_t sequence;
  // redundant information for debugging purposes:
  int64_t price;
  uint64_t id;
  Order(uint64_t quantity, uint64_t sequence, int64_t price, uint64_t id);
  Order(const MarketEvent &e);
};

struct PriceLevel {
  uint64_t total_volume;
  std::list<Order> queue; // sorted in increasing order on sequence number
  friend std::ostream &operator<<(std::ostream &out, const PriceLevel &price) {
    for (const auto &ord : price.queue) {
      out << ord.quantity << " at " << ord.price << "; ID is " << ord.id
          << " and sequence is " << ord.sequence << std::endl;
    }
    return out;
  }
  PriceLevel();
  PriceLevel(const MarketEvent &e);
  PriceLevel(uint64_t volume);
};

class OrderBook {
public:
  void send_event(const MarketEvent &e);
  void debug_print();
  // requests for data about the book to follow
private:
  std::map<int64_t, PriceLevel> bids;
  std::map<int64_t, PriceLevel> asks;
  std::unordered_map<uint64_t, std::list<Order>::iterator> orders_by_id;
  void add_event(const MarketEvent &e);
  void modify_event(const MarketEvent &e);
  void clear();
  void trade_event(const MarketEvent &e);
  void fill_event(const MarketEvent &e);
  void none_event(const MarketEvent &e);
  void cancel_event(const MarketEvent &e);
};
