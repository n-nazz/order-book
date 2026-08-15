#include "event.hpp"

#include <sstream>

MarketEvent::MarketEvent(uint64_t ts_recv, uint32_t size, uint64_t ts_event,
                         uint16_t channel_id, uint8_t rtype, uint64_t order_id,
                         uint16_t publisher_id, uint8_t flags,
                         uint32_t instrument_id, int32_t ts_in_delta,
                         Action action, uint64_t sequence, Side side,
                         std::string symbol, int64_t price,
                         bool is_synthetic) noexcept
    : ts_recv_(ts_recv), size_(size), ts_event_(ts_event),
      channel_id_(channel_id), rtype_(rtype), order_id_(order_id),
      publisher_id_(publisher_id), flags_(flags), instrument_id_(instrument_id),
      ts_in_delta_(ts_in_delta), action_(action), sequence_(sequence),
      side_(side), symbol_(std::move(symbol)), price_(price),
      is_synthetic_(is_synthetic) {}

uint64_t MarketEvent::ts_recv() const noexcept { return ts_recv_; }
uint32_t MarketEvent::size() const noexcept { return size_; }
uint64_t MarketEvent::ts_event() const noexcept { return ts_event_; }
uint16_t MarketEvent::channel_id() const noexcept { return channel_id_; }
uint8_t MarketEvent::rtype() const noexcept { return rtype_; }
uint64_t MarketEvent::order_id() const noexcept { return order_id_; }
uint16_t MarketEvent::publisher_id() const noexcept { return publisher_id_; }
uint8_t MarketEvent::flags() const noexcept { return flags_; }
uint32_t MarketEvent::instrument_id() const noexcept { return instrument_id_; }
int32_t MarketEvent::ts_in_delta() const noexcept { return ts_in_delta_; }
Action MarketEvent::action() const noexcept { return action_; }
uint64_t MarketEvent::sequence() const noexcept { return sequence_; }
Side MarketEvent::side() const noexcept { return side_; }
const std::string &MarketEvent::symbol() const noexcept { return symbol_; }
int64_t MarketEvent::price() const noexcept { return price_; }
bool MarketEvent::is_synthetic() const noexcept { return is_synthetic_; }

std::string MarketEvent::to_string() const {
  std::ostringstream oss;
  oss << "MarketEvent{symbol=" << symbol_ << ", order_id=" << order_id_
      << ", sequence=" << sequence_ << ", price=" << price_
      << ", size=" << size_ << ", ts_recv=" << ts_recv_
      << ", ts_event=" << ts_event_ << ", is_synthetic=" << is_synthetic_
      << "}";
  return oss.str();
}
