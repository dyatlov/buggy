/*
* Copyright 2018 Vitaly Dyatlov
*/
#include <aio.h>

#include <chrono>
#include <functional>
#include <iostream>
#include <thread>
#include <vector>

#include "hhwheel_timer.h"

class EventLoop {
 private:
  buggy::HHWheelTimer _hhwheel_timer;
  std::chrono::steady_clock::time_point _last_timer_run;
 public:
  EventLoop() {
  }
  buggy::HHWheelTimer::TokenPtr set_timeout(std::function<void()> f, int ms) {
    return _hhwheel_timer.set_timeout(f, std::chrono::milliseconds(ms));
  }
  void clear_timeout(buggy::HHWheelTimer::TokenPtr token) {
    _hhwheel_timer.clear_timeout(token);
  }
  void run() {
    _last_timer_run = std::chrono::steady_clock::now();
    for (;;) {
      auto cur_point = std::chrono::steady_clock::now();
      if (!this->_hhwheel_timer.size())
        break;
      if (std::chrono::duration_cast<std::chrono::milliseconds>(cur_point - _last_timer_run).count() >= buggy::HHWheelTimer::TICK_MS) {
        this->_hhwheel_timer.tick();
        _last_timer_run = std::chrono::steady_clock::now();
      } else
        std::this_thread::sleep_for(std::chrono::milliseconds(buggy::HHWheelTimer::TICK_MS));
    }
  }
};

using std::cout;
using std::endl;
int main() {
  std::ios::sync_with_stdio(true);
  buggy::HHWheelTimer::TokenPtr t;
  EventLoop loop;
  loop.set_timeout([&]() {
    cout << "1" << endl;
    loop.set_timeout([&]() {
      cout << "2" << endl;
      t = loop.set_timeout([&]() {
        cout << "3" << endl;
      }, 3000);
      loop.set_timeout([&]() {
        loop.clear_timeout(t);
        cout << "cancelled 3" << endl;
      }, 2500);
    }, 2000);
  }, 1000);
  loop.run();
  return 0;
}
