// dP     dP  88888888b dP    dP  888888ba   .88888.   .d888888   888888ba  888888ba  //
// 88   .d8'  88        Y8.  .8P  88    `8b d8'   `8b d8'    88   88    `8b 88    `8b //
// 88aaa8P'  a88aaaa     Y8aa8P  a88aaaa8P' 88     88 88aaaaa88a a88aaaa8P' 88     88 //
// 88   `8b.  88           88     88   `8b. 88     88 88     88   88   `8b. 88     88 //
// 88     88  88           88     88    .88 Y8.   .8P 88     88   88     88 88    .8P //
// dP     dP  88888888P    dP     88888888P  `8888P'  88     88   dP     dP 8888888P  //

#pragma once
#include <Arduino.h>

#include <functional>
#include <utility>

// forward-declaration to avoid including Adafruit_TCA8418.h
class Adafruit_TCA8418;

// ===================== KB CLASS =====================
class PocketmageKB {
 public:
  explicit PocketmageKB(Adafruit_TCA8418& kp) : keypad_(kp) {}

  using KbStateFn = std::function<int()>;

  void setTCA8418EventFlag(volatile bool* flag) {
    TCA8418_event_ = flag;
  }
  void setPrevTimeMillis(volatile int* prevTime) {
    prevTimeMillis_ = prevTime;
  }
  void setKeyboardState(int* kbState) {
    kbState_ = kbState;
  }
  void setKeyboardStateGetter(KbStateFn fn) {
    kbStateFn_ = std::move(fn);
  }

  // Main methods
  void IRAM_ATTR TCA8418_irq();
  char updateKeypress();

 private:
  Adafruit_TCA8418& keypad_;  // class reference to hardware keypad object

  volatile bool* TCA8418_event_ = nullptr;
  int* kbState_ = nullptr;
  KbStateFn kbStateFn_ = nullptr;

  volatile int* prevTimeMillis_ = nullptr;

  int currentKbState() const;
};

void wireKB();
void setupKB();
// Interrupt handler stored in IRAM for fast interrupt response
void IRAM_ATTR KB_irq_handler();
PocketmageKB& KB();
