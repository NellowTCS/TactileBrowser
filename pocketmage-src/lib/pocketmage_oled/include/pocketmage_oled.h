//    .oooooo.   ooooo        oooooooooooo oooooooooo.    //
//   d8P'  `Y8b  `888'        `888'     `8 `888'   `Y8b   //
//  888      888  888          888          888      888  //
//  888      888  888          888oooo8     888      888  //
//  888      888  888          888    "     888      888  //
//  `88b    d88'  888       o  888       o  888     d88'  //
//   `Y8bood8P'  o888ooooood8 o888ooooood8 o888bood8P'    //

#pragma once
#include <Arduino.h>
#include <RTClib.h>
#include <U8g2lib.h>

#include <functional>
#include <utility>
#include <vector>
#pragma region fonts
// U8G2 FONTS
// U8G2_FOR_ADAFRUIT_GFX u8g2Fonts;
// u8g2_font_4x6_mf
// u8g2_font_5x7_mf
// u8g2_font_spleen5x8_mf
// u8g2_font_boutique_bitmap_9x9_tf
// u8g2_font_courR08_tf.h
#pragma endregion

// ===================== OLED CLASS =====================
class PocketmageOled {
 public:
  explicit PocketmageOled(U8G2& u8) : u8g2_(u8) {}

  // Wire up external buffers/state used to read from globals
  using MeasureTextFn =
      std::function<uint16_t(const String&)>;  // returns text width in e-ink pixels
  using KbStateFn = std::function<int()>;
  using MaxCharsFn = std::function<uint16_t()>;

  void setAllLines(std::vector<String>* lines) {
    lines_ = lines;
  }
  void setDynamicScroll(volatile long* scroll) {
    dynamicScroll_ = scroll;
  }
  void setReferenceWidth(uint16_t w) {
    refWidth_ = w;
  }  // E-ink measurement
  void setMeasureTextWidth(MeasureTextFn fn) {
    measure_ = std::move(fn);
  }  // function for measuring text width in e-ink pixels
  void setBattery(volatile int* st, const uint8_t* const* icons,
                  int iconCount) {  // Battery icon/state
    battState_ = st;
    battIcons_ = icons;
    battIconCount_ = iconCount;
  }
  void setKeyboardState(int* kbState) {
    kbState_ = kbState;
  }  // Keyboard state: 0=NORMAL, 1=SHIFT, 2=FUNC
  void setKeyboardStateGetter(KbStateFn fn) {
    kbStateFn_ = std::move(fn);
  }
  void setClock(RTC_PCF8563* rtc, bool* systemClock, bool* showYear, const char (*days)[12]) {
    rtc_ = rtc;
    systemClock_ = systemClock;
    showYear_ = showYear;
    days_ = days;
  }  // Clock
  void setMSC(bool* mscEnabled) {
    mscEnabled_ = mscEnabled;
  }  // Flags
  void setSD(volatile bool* sdActive) {
    sdActive_ = sdActive;
  }
  void setScrollBitmap(const uint8_t* bmp128x32) {
    scrollBmp_ = bmp128x32;
  }
  void setMaxCharsPerLineEinkGetter(MaxCharsFn fn) {
    maxCharsFn_ = std::move(fn);
  }

  // Main methods
  void oledWord(String word, bool allowLarge = false, bool showInfo = true);
  void oledLine(String line, bool doProgressBar = true, String bottomMsg = "");
  void oledScroll();
  void infoBar();

 private:
  U8G2& u8g2_;  // class reference to hardware oled object

  std::vector<String>* lines_ = nullptr;
  volatile long* dynamicScroll_ = nullptr;

  volatile int* battState_ = nullptr;
  const uint8_t* const* battIcons_ = nullptr;
  int battIconCount_ = 0;

  int* kbState_ = nullptr;
  KbStateFn kbStateFn_;

  RTC_PCF8563* rtc_ = nullptr;
  bool* systemClock_ = nullptr;
  bool* showYear_ = nullptr;
  const char (*days_)[12] = nullptr;

  bool* mscEnabled_ = nullptr;
  volatile bool* sdActive_ = nullptr;

  const uint8_t* scrollBmp_ = nullptr;

  uint16_t refWidth_ = 320;
  MeasureTextFn measure_;  // measure of display text width in e-ink pixels
  MaxCharsFn maxCharsFn_;  // measure
  // helpers
  uint16_t strWidth(const String& s) const;
  int currentKbState() const;
};

void wireOled();
void setupOled();
PocketmageOled& OLED();