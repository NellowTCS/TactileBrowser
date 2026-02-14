#pragma once
#include <Arduino.h>
#include <FS.h>

#include <vector>

class Adafruit_MPR121;
class PocketmageEink;

// ===================== CAPACATIVE TOUCH CLASS =====================
class PocketmageTOUCH {
 public:
  explicit PocketmageTOUCH(Adafruit_MPR121& cap) : cap_(cap) {}

  // Wire up external buffers/state used to read from globals
  void setAllLines(std::vector<String>* allLines) {
    allLines_ = allLines;
  }
  void setEink(PocketmageEink* eink) {
    eink_ = eink;
  }  // reference to pocketmage eink object
  void setEink(PocketmageEink& eink) {
    eink_ = &eink;
  }  // overloaded reference to pocketmage eink object
  void setNewLineAdded(volatile bool* newLineAdded) {
    newLineAdded_ = newLineAdded;
  }
  void setDynamicScroll(volatile long int* dynamicScroll) {
    dynamicScroll_ = dynamicScroll;
  }
  void setPrevDynamicScroll(volatile long int* prev_dynamicScroll) {
    prev_dynamicScroll_ = prev_dynamicScroll;
  }
  void setLastTouch(int* lastTouch) {
    lastTouch_ = lastTouch;
  }
  void setLastTouchTime(unsigned long* lastTouchTime) {
    lastTouchTime_ = lastTouchTime;
  }
  // Main methods
  void updateScrollFromTouch();
  bool updateScroll(int maxScroll, ulong& lineScroll);
  // getters

 private:
  Adafruit_MPR121& cap_;  // class reference to hardware touch object
  PocketmageEink* eink_ = nullptr;
  std::vector<String>* allLines_ = nullptr;
  volatile bool* newLineAdded_ = nullptr;
  volatile long int* dynamicScroll_ = nullptr;
  volatile long int* prev_dynamicScroll_ = nullptr;
  int* lastTouch_ = nullptr;
  unsigned long* lastTouchTime_ = nullptr;
};

void wireTouch();
void setupTouch();
PocketmageTOUCH& TOUCH();