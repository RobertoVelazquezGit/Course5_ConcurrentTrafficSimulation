#include "TrafficLight.h"
#include <chrono> // FP2.a
#include <iostream>
#include <random>

//#define MY_THREAD_ID_DBG 
#ifdef MY_THREAD_ID_DBG
#include <sstream>  // ostringstream
#endif

/* Implementation of class "MessageQueue" */

template <typename T> T MessageQueue<T>::receive() {
  // FP.5a : The method receive should use std::unique_lock<std::mutex>
  // and_condition.wait() to wait for and receive new messages and pull them
  // from the queue using move semantics. The received object should then be
  // returned by the receive function.
  std::unique_lock<std::mutex> lock(_mutex);
  _cond.wait(lock, [this] { return !_queue.empty(); });
  T message = std::move(_queue.front());
  _queue.pop_front();
  return message;
}

template <typename T> void MessageQueue<T>::send(T &&msg) {
  // FP.4a : The method send should use the mechanisms
  // std::lock_guard<std::mutex> as well as _condition.notify_one() to add a new
  // message to the queue and afterwards send a notification.
  std::lock_guard<std::mutex> lck(_mutex);
  _queue.clear();
  _queue.emplace_back(std::move(msg));
  _cond.notify_one();
}

/* Implementation of class "TrafficLight" */

TrafficLight::TrafficLight()
{
    _currentPhase = TrafficLightPhase::red;
}

void TrafficLight::waitForGreen()
{
    // FP.5b : add the implementation of the method waitForGreen, in which an infinite while-loop
    // runs and repeatedly calls the receive function on the message queue.
    // Once it receives TrafficLightPhase::green, the method returns.
    while (true){
        if (TrafficLightPhase::green == _messages.receive()){
            return;
        }
    }    
}

TrafficLightPhase TrafficLight::getCurrentPhase()
{
    return _currentPhase;
}

void TrafficLight::simulate() {
  // FP.2b : Finally, the private method „cycleThroughPhases“ should be
  // started in a thread when the public method „simulate“ is called. To do
  // this, use the thread queue in the base class.
  threads.emplace_back(std::thread(&TrafficLight::cycleThroughPhases, this));
}

// virtual function which is executed in a thread
void TrafficLight::cycleThroughPhases() {
  // FP.2a : Implement the function with an infinite loop that measures the time
  // between two loop cycles and toggles the current phase of the traffic light
  // between red and green and sends an update method to the message queue using
  // move semantics. The cycle duration should be a random value between 4 and 6
  // seconds. Also, the while-loop should use std::this_thread::sleep_for to
  // wait 1ms between two cycles.
  std::random_device rdevice;
  std::uniform_int_distribution<int> unifdist(
      4000, 6000); // random numbers [4000, 6000]
  const unsigned long cycle_time_threshold = unifdist(rdevice);

  #ifdef MY_THREAD_ID_DBG
// gdb ./traffic_simulation
// set environment LD_PRELOAD=/lib/x86_64-linux-gnu/libpthread.so.0
// show environment LD_PRELOAD
// run
//   stops at auto starttime, after __asm__
// info threads
// print target_id
// print thread_id_str

  // Break point in a thread id
  auto this_id = std::this_thread::get_id();
  std::ostringstream oss;
  oss << this_id;  // Convert ID to string
  std::string thread_id_str = oss.str();

  // ID we want to stop (string)
  std::string target_id = "140737017202368";

  if (thread_id_str == target_id) {
      std::cout << ">>> Break here. Target thread reached: " << thread_id_str << std::endl;
      __asm__("int $3");  // To break in gdb
  }
  #endif
  
  auto starttime = std::chrono::system_clock::now();
  while (true) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1)); // wait 1 ms
    const auto endtime = std::chrono::system_clock::now();
    const auto durationtime = std::chrono::duration_cast<std::chrono::milliseconds>(
                            endtime - starttime)
                            .count();
    if (durationtime > cycle_time_threshold) {
      if (TrafficLightPhase::red == _currentPhase) {
        _currentPhase = TrafficLightPhase::green;
      } else {
        _currentPhase = TrafficLightPhase::red;
      }

      // next
      starttime = std::chrono::system_clock::now();

      // Add to queue (FP4.b)
      _messages.send(std::move(_currentPhase));
    }
  }
}