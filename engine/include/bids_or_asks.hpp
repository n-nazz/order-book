#pragma once

#include "event.hpp"
#include <cstddef>
#include <cstdint>
#include <list>
#include <ostream>
#include <vector>

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

inline constexpr size_t kNone = static_cast<size_t>(-1);

struct PriceLevelNode {
  size_t next = kNone, prev = kNone;
  PriceLevel level;
};

// one side (bids or asks) of the book: a vector of PriceLevels indexed by
// (price - minPrice) / tick_size, with the occupied indices threaded
// together in ascending-price order via next/prev so iteration doesn't have
// to scan empty slots.
class BidsOrAsks {
public:
  // asks == true sorts best-first as ascending price; asks == false (bids)
  // sorts best-first as descending price. [minPrice, maxPrice] sets the
  // initial vector span; it grows via fit_vector_to_prices if a price
  // outside that range shows up.
  BidsOrAsks(bool asks, int64_t minPrice, int64_t maxPrice, size_t tick_size);

  size_t size() const { return size_; }
  PriceLevel &operator[](int64_t price);
  int64_t nth_best_price(size_t n) const;

  std::list<Order>::iterator insert_order(const MarketEvent &e);
  bool cancel_order(int64_t price, std::list<Order>::iterator it,
                     uint64_t qty); // returns true if the order was fully cancelled
  void modify_order(int64_t price, std::list<Order>::iterator it,
                     uint64_t new_size);
  void move_order(int64_t old_price, int64_t new_price,
                   std::list<Order>::iterator it, uint64_t new_size,
                   uint64_t new_sequence);
  void clear();

private:
  std::vector<PriceLevelNode> levels;
  size_t headIndex = kNone, tailIndex = kNone, size_ = 0;
  size_t tick_size_;
  int64_t minPrice_, maxPrice_;
  bool asks_;

  size_t index_of(int64_t price) const;
  bool is_occupied(size_t idx) const;
  void link(size_t idx);
  void unlink(size_t idx);
  void fit_vector_to_prices(int64_t newPrice);
};
