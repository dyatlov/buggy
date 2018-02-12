/*
* Copyright 2018 Vitaly Dyatlov
*/
#include "hhwheel_timer.h"

#include <utility>

namespace buggy {

HHWheelTimer::HHWheelTimer()
  : _size(0),
    _last_tick(0),
    start_time(Clock::now()) {
}

HHWheelTimer::CallbackList* HHWheelTimer::rel_ticks_to_bucket(uint64_t ticks) {
  int level = 0, bucket = 0;
  while (level < WHEEL_LEVELS) {
    if (ticks < 1 << WHEEL_BITS * (level + 1)) {
      bucket = ticks >> (WHEEL_BITS * level);
      break;
    }
    ++level;
  }
  bucket += (this->_last_tick >> WHEEL_BITS * level) & (WHEEL_BUCKETS - 1);
  if (bucket >= WHEEL_BUCKETS) {
    ++level;
    bucket = 0;
  }
  if (level >= WHEEL_LEVELS) {
    level = WHEEL_LEVELS - 1;
    bucket = WHEEL_BUCKETS - 1;
  }

  return &buckets[level * WHEEL_BUCKETS + bucket];
}

HHWheelTimer::TokenPtr HHWheelTimer::set_timeout(FuncPtr fn, Ms delay) {
  // minimum schedule is a next loop
  if (delay.count() < TICK_MS)
    delay = Ms(TICK_MS);
  auto expiry_time = Clock::now() + delay;

  uint64_t ticks = delay.count() / TICK_MS;

  auto bucket = rel_ticks_to_bucket(ticks);

  TokenPtr token(new Token());

  auto cb = std::make_unique<HHWheelTimer::Callback>(token, fn, expiry_time);
  bucket->push_front(std::move(cb));

  token->bucket_ptr = bucket;
  token->iter = bucket->begin();
  token->is_valid = true;

  ++_size;

  return token;
}

void HHWheelTimer::clear_timeout(TokenPtr token) {
  if (!token->valid())
    return;
  auto bucket = token->bucket_ptr;
  bucket->erase(token->iter);
  --_size;
}

void HHWheelTimer::cascade_timers(int level, uint64_t tick) {
  CallbackList temp;
  temp.swap(buckets[level * WHEEL_BUCKETS + tick]);
  auto cur_time = Clock::now();
  while (!temp.empty()) {
    auto cb = std::move(temp.back());
    uint64_t ticks;
    if (cb->expiry_time <= cur_time) {
      ticks = 1;  // schedule to next loop
    } else {
      ticks = std::chrono::duration_cast<Ms>(cb->expiry_time
                                             - cur_time).count() / TICK_MS;
    }
    auto nb = rel_ticks_to_bucket(ticks);
    auto token = cb->token;
    nb->push_front(std::move(cb));
    token->bucket_ptr = nb;
    token->iter = nb->begin();
    temp.pop_back();
  }
}

void HHWheelTimer::tick() {
  uint64_t cur_tick = std::chrono::duration_cast<Ms>(Clock::now()
                      - this->start_time).count() / TICK_MS;
  while (_last_tick < cur_tick) {
    auto bucket = &buckets[_last_tick & (WHEEL_BUCKETS - 1)];
    while (!bucket->empty()) {
      std::move(bucket->back())->fn();
      bucket->pop_back();
      --_size;
    }
    ++_last_tick;
    if (!(_last_tick & (WHEEL_BUCKETS - 1))) {
      for (int i = 1; i < WHEEL_LEVELS; i++) {
        uint64_t tick = (_last_tick >> (WHEEL_BITS * i)) & (WHEEL_BUCKETS - 1);
        cascade_timers(i, tick);
        if (tick)
          break;
        // reset values
        if (i + 1 == WHEEL_LEVELS) {
          cur_tick -= _last_tick;
          _last_tick = 0;
          start_time = Clock::now();
        }
      }
    }
  }

  _last_tick = cur_tick;
}

uint64_t HHWheelTimer::size() const {
  return _size;
}

}  // namespace buggy
