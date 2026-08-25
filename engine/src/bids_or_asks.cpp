#include "bids_or_asks.hpp"
#include <algorithm>
#include <iterator>

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

BidsOrAsks::BidsOrAsks(bool asks, int64_t minPrice, int64_t maxPrice,
                        size_t tick_size)
    : levels((maxPrice - minPrice) / tick_size + 1), tick_size_(tick_size),
      minPrice_(minPrice), maxPrice_(maxPrice), asks_(asks) {}

size_t BidsOrAsks::index_of(int64_t price) const {
  return static_cast<size_t>((price - minPrice_) /
                              static_cast<int64_t>(tick_size_));
}

bool BidsOrAsks::is_occupied(size_t idx) const {
  return idx == headIndex || idx == tailIndex || levels[idx].prev != kNone ||
         levels[idx].next != kNone;
}

void BidsOrAsks::link(size_t idx) {
  PriceLevelNode &node = levels[idx];
  if (headIndex == kNone) {
    node.prev = node.next = kNone;
    headIndex = tailIndex = idx;
  } else if (idx < headIndex) {
    node.prev = kNone;
    node.next = headIndex;
    levels[headIndex].prev = idx;
    headIndex = idx;
  } else if (idx > tailIndex) {
    node.next = kNone;
    node.prev = tailIndex;
    levels[tailIndex].next = idx;
    tailIndex = idx;
  } else if (idx - headIndex < tailIndex - idx) {
    size_t cur = headIndex;
    while (levels[cur].next < idx)
      cur = levels[cur].next;
    size_t nxt = levels[cur].next;
    node.prev = cur;
    node.next = nxt;
    levels[cur].next = idx;
    levels[nxt].prev = idx;
  } else {
    size_t cur = tailIndex;
    while (levels[cur].prev > idx)
      cur = levels[cur].prev;
    size_t prv = levels[cur].prev;
    node.next = cur;
    node.prev = prv;
    levels[cur].prev = idx;
    levels[prv].next = idx;
  }
  ++size_;
}

void BidsOrAsks::unlink(size_t idx) {
  PriceLevelNode &node = levels[idx];
  if (node.prev == kNone)
    headIndex = node.next;
  else
    levels[node.prev].next = node.next;
  if (node.next == kNone)
    tailIndex = node.prev;
  else
    levels[node.next].prev = node.prev;
  node.prev = node.next = kNone;
  --size_;
}

void BidsOrAsks::fit_vector_to_prices(int64_t newPrice) {
  int64_t tick = static_cast<int64_t>(tick_size_);
  if (newPrice > maxPrice_) {
    size_t old_size = levels.size();
    size_t needed =
        static_cast<size_t>((newPrice - maxPrice_ + tick - 1) / tick);
    size_t new_size = std::max(old_size * 2, old_size + needed);
    levels.resize(new_size);
    maxPrice_ += static_cast<int64_t>(new_size - old_size) * tick;
  } else {
    size_t needed =
        static_cast<size_t>((minPrice_ - newPrice + tick - 1) / tick);
    size_t shift = std::max(levels.size(), needed);
    if (headIndex != kNone) {
      for (size_t idx = headIndex, next; idx != kNone; idx = next) {
        next = levels[idx].next;
        if (levels[idx].prev != kNone)
          levels[idx].prev += shift;
        levels[idx].next = (next != kNone) ? next + shift : kNone;
      }
      headIndex += shift;
      tailIndex += shift;
    }
    levels.insert(levels.begin(), shift, PriceLevelNode());
    minPrice_ -= static_cast<int64_t>(shift) * tick;
  }
}

PriceLevel &BidsOrAsks::operator[](int64_t price) {
  return levels[index_of(price)].level;
}

int64_t BidsOrAsks::nth_best_price(size_t n) const {
  size_t idx = asks_ ? headIndex : tailIndex;
  for (size_t i = 0; i < n; ++i)
    idx = asks_ ? levels[idx].next : levels[idx].prev;
  return minPrice_ +
         static_cast<int64_t>(idx) * static_cast<int64_t>(tick_size_);
}

std::list<Order>::iterator BidsOrAsks::insert_order(const MarketEvent &e) {
  if (e.price() < minPrice_ || e.price() > maxPrice_)
    fit_vector_to_prices(e.price());
  size_t idx = index_of(e.price());
  if (!is_occupied(idx))
    link(idx);
  PriceLevel &level = levels[idx].level;
  level.total_volume += e.size();
  auto pos = level.queue.end();
  while (pos != level.queue.begin() && std::prev(pos)->sequence > e.sequence())
    --pos;
  return level.queue.insert(pos, Order(e));
}

bool BidsOrAsks::cancel_order(int64_t price, std::list<Order>::iterator it,
                               uint64_t qty) {
  size_t idx = index_of(price);
  PriceLevel &level = levels[idx].level;
  it->quantity -= qty;
  level.total_volume -= qty;
  bool fully_cancelled = it->quantity == 0;
  if (fully_cancelled)
    level.queue.erase(it);
  if (level.total_volume == 0) {
    unlink(idx);
    level = PriceLevel();
  }
  return fully_cancelled;
}

void BidsOrAsks::modify_order(int64_t price, std::list<Order>::iterator it,
                               uint64_t new_size) {
  levels[index_of(price)].level.total_volume -= (it->quantity - new_size);
  it->quantity = new_size;
}

void BidsOrAsks::move_order(int64_t old_price, int64_t new_price,
                             std::list<Order>::iterator it, uint64_t new_size,
                             uint64_t new_sequence) {
  if (new_price < minPrice_ || new_price > maxPrice_)
    fit_vector_to_prices(new_price);
  size_t old_idx = index_of(old_price);
  size_t new_idx = index_of(new_price);
  if (!is_occupied(new_idx))
    link(new_idx);
  PriceLevelNode &old_node = levels[old_idx];
  PriceLevelNode &new_node = levels[new_idx];
  new_node.level.total_volume += new_size;
  old_node.level.total_volume -= it->quantity;
  auto pos = new_node.level.queue.end();
  while (pos != new_node.level.queue.begin() &&
         std::prev(pos)->sequence > new_sequence)
    --pos;
  new_node.level.queue.splice(pos, old_node.level.queue, it);
  if (old_node.level.total_volume == 0) {
    unlink(old_idx);
    old_node.level = PriceLevel();
  }
  it->price = new_price;
  it->quantity = new_size;
  it->sequence = new_sequence;
}

void BidsOrAsks::clear() {
  levels.assign(levels.size(), PriceLevelNode());
  headIndex = tailIndex = kNone;
  size_ = 0;
}
