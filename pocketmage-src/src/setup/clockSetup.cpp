#include <pocketmage.h>

static constexpr const char* TAG = "CLOCK";  // TODO: Come up with a better tag

static PocketmageCLOCK pm_clock(rtc);

void setupClock() {
  pinMode(RTC_INT, INPUT);
  if (!CLOCK().begin()) {
    ESP_LOGE(TAG, "Couldn't find RTC");
    delay(1000);
  }
  // SET CLOCK IF NEEDED
  if (SET_CLOCK_ON_UPLOAD || CLOCK().getRTC().lostPower()) {
    CLOCK().setToCompileTimeUTC();
  }
  CLOCK().getRTC().start();
  wireClock();
}

void wireClock() {}

// clock object reference for other apps
PocketmageCLOCK& CLOCK() {
  return pm_clock;
}