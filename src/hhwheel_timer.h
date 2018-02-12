/*
* Copyright 2018 Vitaly Dyatlov
*/
#ifndef _BUGGY_HHWHEEL_TIMER_H
#define _BUGGY_HHWHEEL_TIMER_H

#include <chrono>
#include <functional>
#include <memory>
#include <array>
#include <list>

namespace buggy {

class HHWheelTimer {
  class Callback;
 public:
  class Token;
  typedef std::chrono::steady_clock::time_point TimePoint;
  typedef std::shared_ptr<Token> TokenPtr;
  typedef std::unique_ptr<Callback> CallbackPtr;
  typedef std::list<CallbackPtr> CallbackList;
  typedef std::chrono::milliseconds Ms;
  typedef std::function<void()> FuncPtr;
  typedef std::chrono::steady_clock Clock;
  class Token {
   private:
    Token(): is_valid(false) {}
    void mark_invalid() {
      is_valid = false;
    }
    bool valid() {
      return is_valid;
    }
    bool is_valid;
    CallbackList* bucket_ptr;
    CallbackList::iterator iter;
    friend class HHWheelTimer;
  };
  static constexpr int TICK_MS = 4;
  HHWheelTimer();
  TokenPtr set_timeout(FuncPtr fn,
                       Ms delay);
  void clear_timeout(TokenPtr token);
  uint64_t size() const;
  void tick();
 private:
  class Callback {
   public:
    explicit Callback(TokenPtr token, FuncPtr fn,
                      TimePoint expiry_time) {
      this->token = token;
      this->fn = fn;
      this->expiry_time = expiry_time;
    }
    ~Callback() {
      this->token->mark_invalid();
    }
   private:
    TokenPtr token;
    FuncPtr fn;
    TimePoint expiry_time;
    friend class HHWheelTimer;
  };
  uint64_t _size;
  uint64_t _last_tick;
  TimePoint start_time;
  static constexpr int WHEEL_LEVELS = 4;
  static constexpr int WHEEL_BITS = 8;
  static constexpr int WHEEL_BUCKETS = (1 << WHEEL_BITS);
  std::array<CallbackList, WHEEL_LEVELS*WHEEL_BUCKETS> buckets;
  CallbackList* rel_ticks_to_bucket(uint64_t ticks);
  void cascade_timers(int level, uint64_t tick);
};

}  // namespace buggy

#endif  // _BUGGY_HHWHEEL_TIMER_H
