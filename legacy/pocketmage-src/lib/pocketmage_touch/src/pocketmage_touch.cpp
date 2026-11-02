#include <Adafruit_MPR121.h>
#include <config.h>  // for TOUCH_TIMEOUT_MS
#include <pocketmage_eink.h>
#include <pocketmage_touch.h>

void PocketmageTOUCH::updateScrollFromTouch() {
  uint16_t touched = cap_.touched();
  int newTouch = -1;

  for (int i = 0; i < 9; ++i)
    if (touched & (1 << i)) {
      newTouch = i;
      break;
    }

  unsigned long now = millis();

  if (newTouch != -1) {
    if (*lastTouch_ != -1) {
      int d = abs(newTouch - *lastTouch_);
      if (d <= 2) {
        int maxScroll = max(0, (int)allLines_->size() - eink_->maxLines());
        if (newTouch > *lastTouch_) {
          *dynamicScroll_ = min((long)(*dynamicScroll_ + 1), (long)maxScroll);
        } else if (newTouch < *lastTouch_) {
          *dynamicScroll_ = max((long)(*dynamicScroll_ - 1), 0L);
        }
      }
    }
    *lastTouch_ = newTouch;
    *lastTouchTime_ = now;
  } else if (*lastTouch_ != -1 && (now - *lastTouchTime_ > TOUCH_TIMEOUT_MS)) {
    *lastTouch_ = -1;
    if (*prev_dynamicScroll_ != *dynamicScroll_)
      *newLineAdded_ = true;
  }
}

bool PocketmageTOUCH::updateScroll(int maxScroll, ulong& lineScroll) {
  const char* tag = "TOUCH";
  static int lastTouchPos = -1;
  static unsigned long lastTouchTime = 0;
  static int prev_lineScroll = 0;
  bool updateScreen = false;

  uint16_t touched = cap_.touched();  // Read touch state
  int touchPos = -1;

  // Find the first active touch point (lowest index first)
  for (int i = 0; i < 9; i++) {
    if (touched & (1 << i)) {
      touchPos = i;

      ESP_LOGI(tag, "Prev pad: %d\tTouched pad: %d\n", lastTouchPos,
               touchPos);  // TODO(logging): come up with more descriptive tags

      break;
    }
  }

  unsigned long currentTime = millis();

  if (touchPos != -1) {  // If a touch is detected
    ESP_LOGI(tag, "Touch detected\n");

    if (lastTouchPos != -1) {  // Compare with previous touch
      int touchDelta = abs(touchPos - lastTouchPos);
      if (touchDelta <= 2) {  // Ignore large jumps

        // REVERSED SCROLL DIRECTION:
        if (touchPos < lastTouchPos && lineScroll < maxScroll) {
          prev_lineScroll = lineScroll;
          lineScroll++;
        } else if (touchPos > lastTouchPos && lineScroll > 0) {
          prev_lineScroll = lineScroll;
          lineScroll--;
        }
      }
    }

    lastTouchPos = touchPos;      // update tracked touch
    *lastTouch_ = touchPos;       // <--- update UI flag
    lastTouchTime = currentTime;  // reset timeout
  } else if (lastTouchPos != -1 && (currentTime - lastTouchTime > TOUCH_TIMEOUT_MS)) {
    // Timeout: reset both
    lastTouchPos = -1;
    *lastTouch_ = -1;  // <--- reset UI flag

    if (prev_lineScroll != lineScroll) {
      updateScreen = true;
    }

    prev_lineScroll = lineScroll;
  }
  return updateScreen;
}
