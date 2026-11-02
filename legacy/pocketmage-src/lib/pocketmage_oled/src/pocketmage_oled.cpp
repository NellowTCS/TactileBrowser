//    .oooooo.   ooooo        oooooooooooo oooooooooo.    //
//   d8P'  `Y8b  `888'        `888'     `8 `888'   `Y8b   //
//  888      888  888          888          888      888  //
//  888      888  888          888oooo8     888      888  //
//  888      888  888          888    "     888      888  //
//  `88b    d88'  888       o  888       o  888     d88'  //
//   `Y8bood8P'  o888ooooood8 o888ooooood8 o888bood8P'    //

#include <pocketmage_oled.h>

// ===================== public functions =====================
void PocketmageOled::oledWord(String word, bool allowLarge, bool showInfo) {
  u8g2_.clearBuffer();

  if (showInfo)
    infoBar();

  if (allowLarge) {
    /*u8g2_.setFont(u8g2_font_ncenB24_tr);
    if (u8g2_.getStrWidth(word.c_str()) < u8g2_.getDisplayWidth()) {
      u8g2_.drawStr((u8g2_.getDisplayWidth() -
    u8g2_.getStrWidth(word.c_str()))/2,16+12,word.c_str()); u8g2_.sendBuffer(); return;
    }*/
    u8g2_.setFont(u8g2_font_ncenB18_tr);
    if (u8g2_.getStrWidth(word.c_str()) < u8g2_.getDisplayWidth()) {
      u8g2_.drawStr((u8g2_.getDisplayWidth() - u8g2_.getStrWidth(word.c_str())) / 2, 16 + 9,
                    word.c_str());
      u8g2_.sendBuffer();
      return;
    }
  }

  u8g2_.setFont(u8g2_font_ncenB14_tr);
  if (u8g2_.getStrWidth(word.c_str()) < u8g2_.getDisplayWidth()) {
    u8g2_.drawStr((u8g2_.getDisplayWidth() - u8g2_.getStrWidth(word.c_str())) / 2, 16 + 7,
                  word.c_str());
    u8g2_.sendBuffer();
    return;
  }

  u8g2_.setFont(u8g2_font_ncenB12_tr);
  if (u8g2_.getStrWidth(word.c_str()) < u8g2_.getDisplayWidth()) {
    u8g2_.drawStr((u8g2_.getDisplayWidth() - u8g2_.getStrWidth(word.c_str())) / 2, 16 + 6,
                  word.c_str());
    u8g2_.sendBuffer();
    return;
  }

  u8g2_.setFont(u8g2_font_ncenB10_tr);
  if (u8g2_.getStrWidth(word.c_str()) < u8g2_.getDisplayWidth()) {
    u8g2_.drawStr((u8g2_.getDisplayWidth() - u8g2_.getStrWidth(word.c_str())) / 2, 16 + 5,
                  word.c_str());
    u8g2_.sendBuffer();
    return;
  }

  u8g2_.setFont(u8g2_font_ncenB08_tr);
  if (u8g2_.getStrWidth(word.c_str()) < u8g2_.getDisplayWidth()) {
    u8g2_.drawStr((u8g2_.getDisplayWidth() - u8g2_.getStrWidth(word.c_str())) / 2, 16 + 4,
                  word.c_str());
    u8g2_.sendBuffer();
    return;
  } else {
    u8g2_.drawStr(u8g2_.getDisplayWidth() - u8g2_.getStrWidth(word.c_str()), 16 + 4, word.c_str());
    u8g2_.sendBuffer();
    return;
  }
}

void PocketmageOled::oledLine(String line, bool doProgressBar, String bottomMsg) {
  uint8_t mcl = maxCharsFn_ ? (uint8_t)maxCharsFn_() : /*fallback*/ 29;
  uint8_t maxLength = mcl;
  u8g2_.clearBuffer();

  // PROGRESS BAR
  if (doProgressBar && line.length() > 0) {
    // uint8_t progress = map(line.length(), 0, maxLength, 0, 128);

    const uint16_t charWidth = strWidth(line);

    const uint8_t progress = map(charWidth, 0, (int)refWidth_ - 5, 0, u8g2_.getDisplayWidth());

    u8g2_.drawVLine(u8g2_.getDisplayWidth(), 0, 2);
    u8g2_.drawVLine(0, 0, 2);

    u8g2_.drawHLine(0, 0, progress);
    u8g2_.drawHLine(0, 1, progress);

    // LINE END WARNING INDICATOR
    if (charWidth > ((refWidth_ - 5) * 0.8)) {
      if ((millis() / 400) % 2 == 0) {  // ON for 200ms, OFF for 200ms
        u8g2_.drawVLine(u8g2_.getDisplayWidth() - 1, 8, 32 - 16);
        u8g2_.drawLine(u8g2_.getDisplayWidth() - 1, 15, u8g2_.getDisplayWidth() - 4, 12);
        u8g2_.drawLine(u8g2_.getDisplayWidth() - 1, 15, u8g2_.getDisplayWidth() - 4, 18);
      }
    }
  }

  // No bottom msg, show infobar
  if (bottomMsg.length() == 0) {
    infoBar();
  }
  // Display bottomMsg
  else {
    u8g2_.setFont(u8g2_font_5x7_tf);
    u8g2_.drawStr(0, u8g2_.getDisplayHeight(), bottomMsg.c_str());

    // Draw FN/Shift indicator
    int state = currentKbState();
    switch (state) {
      case 1:  // SHIFT
        u8g2_.drawStr((u8g2_.getDisplayWidth() - u8g2_.getStrWidth("SHIFT")),
                      u8g2_.getDisplayHeight(), "SHIFT");
        break;
      case 2:  // FUNC
        u8g2_.drawStr((u8g2_.getDisplayWidth() - u8g2_.getStrWidth("FN")), u8g2_.getDisplayHeight(),
                      "FN");
        break;
      default:
        break;
    }
  }

  // DRAW LINE TEXT (unchanged)
  u8g2_.setFont(u8g2_font_ncenB18_tr);
  if (u8g2_.getStrWidth(line.c_str()) < (u8g2_.getDisplayWidth() - 5)) {
    u8g2_.drawStr(0, 20, line.c_str());
    if (line.length() > 0)
      u8g2_.drawVLine(u8g2_.getStrWidth(line.c_str()) + 2, 1, 22);
  } else {
    u8g2_.drawStr(u8g2_.getDisplayWidth() - 8 - u8g2_.getStrWidth(line.c_str()), 20, line.c_str());
  }

  u8g2_.sendBuffer();
}

void PocketmageOled::infoBar() {
  // FN/SHIFT indicator centered
  u8g2_.setFont(u8g2_font_5x7_tf);

  int state = currentKbState();

  switch (state) {
    case 1:
      u8g2_.drawStr((u8g2_.getDisplayWidth() - u8g2_.getStrWidth("SHIFT")) / 2,
                    u8g2_.getDisplayHeight(), "SHIFT");
      break;
    case 2:
      u8g2_.drawStr((u8g2_.getDisplayWidth() - u8g2_.getStrWidth("FN")) / 2,
                    u8g2_.getDisplayHeight(), "FN");
      break;
    default:
      break;
  }

  int infoWidth = 16;

  // Battery Indicator
  if (battIcons_ && battState_ && battIconCount_ > 0) {
    int maxIconIndex = battIconCount_ - 1;
    int state = *battState_;
    state = (int)constrain(state, 0, maxIconIndex);
    u8g2_.drawXBMP(0, u8g2_.getDisplayHeight() - 6, 10, 6, battIcons_[state]);
  }

  // CLOCK
  if (systemClock_ && *systemClock_ && rtc_) {
    u8g2_.setFont(u8g2_font_5x7_tf);
    DateTime now = rtc_->now();

    // shortened time format
    String timeString =
        String(now.hour()) + ":" + (now.minute() < 10 ? "0" : "") + String(now.minute());
    u8g2_.drawStr(infoWidth, u8g2_.getDisplayHeight(), timeString.c_str());

    String day3Char = days_ ? String(days_[now.dayOfTheWeek()]) : String("Day");
    day3Char = day3Char.substring(0, 3);
    if (showYear_ && *showYear_)
      day3Char += (" " + String(now.month()) + "/" + String(now.day()) + "/" +
                   String(now.year()).substring(2, 4));
    else
      day3Char += (" " + String(now.month()) + "/" + String(now.day()));
    u8g2_.drawStr(u8g2_.getDisplayWidth() - u8g2_.getStrWidth(day3Char.c_str()),
                  u8g2_.getDisplayHeight(), day3Char.c_str());

    infoWidth += (u8g2_.getStrWidth(timeString.c_str()) + 6);
  }

  // MSC Indicator
  if (mscEnabled_ && *mscEnabled_) {
    u8g2_.setFont(u8g2_font_5x7_tf);
    u8g2_.drawStr(infoWidth, u8g2_.getDisplayHeight(), "USB");

    infoWidth += (u8g2_.getStrWidth("USB") + 6);
  }

  // SD Indicator
  if (sdActive_ && *sdActive_) {
    u8g2_.setFont(u8g2_font_5x7_tf);
    u8g2_.drawStr(infoWidth, u8g2_.getDisplayHeight(), "SD");

    infoWidth += (u8g2_.getStrWidth("SD") + 6);
  }
}

void PocketmageOled::oledScroll() {
  // CLEAR DISPLAY
  u8g2_.clearBuffer();

  // DRAW BACKGROUND
  if (scrollBmp_)
    u8g2_.drawXBMP(0, 0, 128, 32, scrollBmp_);

  // DRAW LINES PREVIEW
  const long count = lines_->size();
  const long startIndex = max((long)(count - *dynamicScroll_), 0L);
  const long endIndex = max((long)(count - *dynamicScroll_ - 9), 0L);

  // CHECK IF LINE STARTS WITH A TAB
  for (long i = startIndex; i > endIndex && i >= 0; --i) {
    if (i >= count)
      continue;  // Ensure i is within bounds

    // CHECK IF LINE STARTS WITH A TAB
    const bool tabbed = (*lines_)[i].startsWith("    ");
    const String& s = tabbed ? (*lines_)[i].substring(4) : (*lines_)[i];
    const uint16_t w = strWidth(s);

    // ADJUST DRAW COORDINATES BASED ON TAB
    const int refMax = tabbed ? 49 : 56;
    const int lineW = constrain(map((int)w, 0, 320, 0, refMax), 0, refMax);
    const int y = 28 - (4 * (startIndex - i));
    const int x = tabbed ? 68 : 61;

    u8g2_.drawBox(x, y, lineW, 2);
  }

  // PRINT CURRENT LINE
  u8g2_.setFont(u8g2_font_ncenB08_tr);
  String lineNumStr = String(startIndex) + "/" + String(count);
  u8g2_.drawStr(0, 12, "Line:");
  u8g2_.drawStr(0, 24, lineNumStr.c_str());

  // PRINT LINE PREVIEW
  if (startIndex >= 0 && startIndex < lines_->size()) {
    const String& line = (*lines_)[startIndex];
    if (line.length() > 0) {
      u8g2_.setFont(u8g2_font_ncenB18_tr);
      u8g2_.drawStr(140, 24, line.c_str());
    }
  }

  // SEND BUFFER
  u8g2_.sendBuffer();
}

// ===================== private functions =====================
// COMPUTE STRING WIDTH IN EINK PIXELS
uint16_t PocketmageOled::strWidth(const String& s) const {
  // Fallback: map u8g2 width to the reference width
  if (measure_)
    return measure_(s);
  float scale = refWidth_ / (float)u8g2_.getDisplayWidth();
  return (uint16_t)(u8g2_.getStrWidth(s.c_str()) * scale);
}
// REFERENCE CURRENT KEYBOARD STATE
int PocketmageOled::currentKbState() const {
  if (kbStateFn_)
    return kbStateFn_();
  if (kbState_)
    return *kbState_;
  return 0;
}
