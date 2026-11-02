#include <pocketmage.h>

// Initialization of capacative touch class
static PocketmageTOUCH pm_touch(cap);

static constexpr const char* TAG = "TOUCH";

void setupTouch() {
  // MPR121 / SLIDER
  if (!cap.begin(MPR121_ADDR)) {
    ESP_LOGE(TAG, "TouchPad Failed");
    OLED().oledWord("TouchPad Failed");
    delay(1000);
  }
  cap.setAutoconfig(true);
  wireTouch();
}

void wireTouch() {
  pm_touch.setAllLines(&allLines);
  pm_touch.setEink(EINK());
  pm_touch.setNewLineAdded(&newLineAdded);
  pm_touch.setDynamicScroll(&dynamicScroll);
  pm_touch.setPrevDynamicScroll(&prev_dynamicScroll);
  pm_touch.setLastTouch(&lastTouch);
  pm_touch.setLastTouchTime(&lastTouchTime);
}

// touch object reference for other apps
PocketmageTOUCH& TOUCH() {
  return pm_touch;
}