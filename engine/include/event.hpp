#pragma once

#include <cstdint>
#include <string>

enum class Action : uint8_t { Add, Cancel, Modify, Clear, Trade, Fill, None };

enum class Side : uint8_t { Ask, Bid, None };

class MarketEvent {
public:
  MarketEvent(uint64_t ts_recv, uint32_t size, uint64_t ts_event,
              uint16_t channel_id, uint8_t rtype, uint64_t order_id,
              uint16_t publisher_id, uint8_t flags, uint32_t instrument_id,
              int32_t ts_in_delta, Action action, uint64_t sequence, Side side,
              std::string symbol, int64_t price,
              bool is_synthetic = false) noexcept;

  uint64_t ts_recv() const noexcept;
  uint32_t size() const noexcept;
  uint64_t ts_event() const noexcept;
  uint16_t channel_id() const noexcept;
  uint8_t rtype() const noexcept;
  uint64_t order_id() const noexcept;
  uint16_t publisher_id() const noexcept;
  uint8_t flags() const noexcept;
  uint32_t instrument_id() const noexcept;
  int32_t ts_in_delta() const noexcept;
  Action action() const noexcept;
  uint64_t sequence() const noexcept;
  Side side() const noexcept;
  const std::string &symbol() const noexcept;
  int64_t price() const noexcept;
  bool is_synthetic() const noexcept;

  std::string to_string() const;

private:
  uint64_t ts_recv_;
  uint32_t size_;
  uint64_t ts_event_;
  uint16_t channel_id_;
  uint8_t rtype_;
  uint64_t order_id_;
  uint16_t publisher_id_;
  uint8_t flags_;
  uint32_t instrument_id_;
  int32_t ts_in_delta_;
  Action action_;
  uint64_t sequence_;
  Side side_;
  std::string symbol_;
  int64_t price_;
  bool is_synthetic_;
};
