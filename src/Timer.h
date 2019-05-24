#ifndef PROJECT_TIMER_H
#define PROJECT_TIMER_H

#include <thread>
#include <chrono>
#include <memory>
#include <mutex>
#include <condition_variable>


#include <iostream>


template<typename Fun>
class Timer {
  int milliseconds{};
  Fun fun;

  bool detach{};

  volatile bool stopped{};

  int delta_check_ms{};

  std::unique_ptr<std::thread> timer_thread;

  std::mutex mutex;
  std::condition_variable condition_variable;

  bool started = false;

  bool finished = false;


public:

  Timer()= default;




  Timer(int miliseconds, Fun fun, int delta_check_ms = 100, bool detach = false) :
          milliseconds(miliseconds), fun(fun), stopped(false), delta_check_ms(delta_check_ms), detach(detach){
    start();
  }

  ~Timer() {
    if (!started || !timer_thread || !timer_thread->joinable()) {
      return;
    }

    if (detach) {
      timer_thread->detach();
    } else {
      timer_thread->join();
    }
  }

  bool has_started() {
    return started;
  }

  void setDetachOn() {
    detach = true;
  }

  void start() {
    started = true;
    if (delta_check_ms > milliseconds) {
      delta_check_ms = milliseconds;
    }
    auto &f = fun;
    timer_thread = std::make_unique<std::thread>([
                                                         m = milliseconds,
                                                         &f = fun,
                                                         dc = delta_check_ms,
                                                         mx = &mutex,
                                                         condition_variable = &condition_variable,
                                                         _stopped = &stopped,
                                                         finished = &finished]() {
      std::unique_lock<std::mutex> lock(*mx);
      condition_variable->wait_for(lock, std::chrono::milliseconds(m), [_stopped = _stopped]() { return *_stopped; });

      if (*_stopped) {
        *finished = true;
        return;
      }

      *_stopped = true;
      lock.unlock();
      condition_variable->notify_all();
      f();
      *finished = true;
    });
  }

  void stop() {
    if (!has_started() || is_stopped() || is_finished()) {
      return;
    }
    {
      std::lock_guard<std::mutex> lock(mutex);
      stopped = true;
    }
    condition_variable.notify_all();
  }

  bool is_stopped() {
    bool was_stopped;
    {
      std::lock_guard<std::mutex> lock(mutex);
      was_stopped = stopped;
    }

    return was_stopped;
  }

  bool is_finished() {
    std::lock_guard<std::mutex> lock(mutex);
    return finished;

  }



};


template<typename Fun>
std::unique_ptr<Timer<Fun>> startTimeout(int milliseconds, Fun &fun, bool detach = false) {
  return std::make_unique<Timer<Fun>>(milliseconds, fun, detach);
}

template<typename Fun>
std::unique_ptr<Timer<Fun>> startTimeout(int milliseconds, Fun &&fun, bool detach = false) {
  return std::make_unique<Timer<Fun>>(milliseconds, fun, detach);
}


#endif //PROJECT_TIMER_H
