#include "pocketmage_clock.h"

bool PocketmageCLOCK::begin() {
  if (!rtc_.begin()) {
    begun_ = false;
    return false;
  }
  begun_ = true;
  return true;
}

bool PocketmageCLOCK::isValid() {
  if (!begun_)
    return false;
  DateTime t = rtc_.now();
  const bool saneYear =
      t.year() >= 2020 && t.year() < 2099;  // check for reasonable year for DateTime t
  return saneYear;
}
