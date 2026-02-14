#pragma once
#include <Arduino.h>
#include <RTClib.h>

class PocketmageCLOCK {
 public:
  explicit PocketmageCLOCK(RTC_PCF8563& rtc) : rtc_(rtc) {}

  bool begin();
  bool isValid();

  void setToCompileTimeUTC() {
    rtc_.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  DateTime nowDT() {
    return rtc_.now();
  }
  RTC_PCF8563& getRTC() {
    return rtc_;
  }

 private:
  RTC_PCF8563& rtc_;
  bool begun_ = false;
};

void wireClock();
void setupClock();
PocketmageCLOCK& CLOCK();
