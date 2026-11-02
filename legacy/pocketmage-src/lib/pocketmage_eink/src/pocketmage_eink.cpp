//  oooooooooooo         ooooo ooooo      ooo oooo    oooo  //
//  `888'     `8         `888' `888b.     `8' `888   .8P'   //
//   888                  888   8 `88b.    8   888  d8'     //
//   888oooo8    8888888  888   8   `88b.  8   88888[       //
//   888    "             888   8     `88b.8   888`88b.     //
//   888       o          888   8       `888   888  `88b.   //
//  o888ooooood8         o888o o8o        `8  o888o  o888o  //

#include <pocketmage_eink.h>
PocketmageEink::PocketmageEink(DisplayT& d) : display_(d) {}

// ===================== main functions =====================
void PocketmageEink::refresh() {
  // USE A SLOW FULL UPDATE EVERY N FAST UPDATES OR WHEN SPECIFIED
  if ((partialCounter_ >= fullRefreshAfter_) || forceSlowFullUpdate_) {
    forceSlowFullUpdate_ = false;
    partialCounter_ = 0;
    setFastFullRefresh(false);
  }
  // OTHERWISE USE A FAST FULL UPDATE
  else {
    setFastFullRefresh(true);
    partialCounter_++;
  }

  display_.display(false);

  display_.setFullWindow();
  display_.fillScreen(GxEPD_WHITE);
  display_.hibernate();
}
void PocketmageEink::multiPassRefresh(int passes) {
  display_.display(false);
  if (passes > 0) {
    for (int i = 0; i < passes; i++) {
      delay(250);
      display_.display(true);
    }
  }

  delay(100);
  display_.setFullWindow();
  display_.fillScreen(GxEPD_WHITE);
  display_.hibernate();
}
void PocketmageEink::setFastFullRefresh(bool setting) {
  PanelT::useFastFullUpdate = setting;
  /*if (PanelT::useFastFullUpdate != setting) {
    PanelT::useFastFullUpdate = setting;
  }*/
}
void PocketmageEink::statusBar(const String& input, bool fullWindow) {
  setTXTFont(&FreeMonoBold9pt7b);
  if (!fullWindow) {
    display_.setPartialWindow(0, display_.height() - 20, display_.width(), 20);
    display_.fillRect(0, display_.height() - 26, display_.width(), 26, GxEPD_WHITE);
    display_.drawRect(0, display_.height() - 20, display_.width(), 20, GxEPD_BLACK);
    display_.setCursor(4, display_.height() - 6);
    display_.print(input);
  }

  display_.drawRect(display_.width() - 30, display_.height() - 20, 30, 20, GxEPD_BLACK);
}
void PocketmageEink::drawStatusBar(const String& input) {
  display_.fillRect(0, display_.height() - 26, display_.width(), 26, GxEPD_WHITE);
  display_.drawRect(0, display_.height() - 20, display_.width(), 20, GxEPD_BLACK);
  setTXTFont(&FreeMonoBold9pt7b);
  display_.setCursor(4, display_.height() - 6);
  display_.print(input);
}
void PocketmageEink::computeFontMetrics_() {
  int16_t x1, y1;
  uint16_t charWidth, charHeight;
  // GET AVERAGE CHAR WIDTH
  display_.getTextBounds("abcdefghijklmnopqrstuvwxyz", 0, 0, &x1, &y1, &charWidth, &charHeight);
  charWidth = charWidth / 52;  // check if intended
  maxCharsPerLine_ = display_.width() / charWidth;

  display_.getTextBounds("H", 0, 0, &x1, &y1, &charWidth, &charHeight);
  fontHeight_ = charHeight;
  maxLines_ = (display_.height() - 26) / (fontHeight_ + lineSpacing_);
}
void PocketmageEink::setTXTFont(const GFXfont* font) {
  // SET THE FONT
  const bool changed = (currentFont_ != font);
  currentFont_ = font;
  display_.setFont(currentFont_);
  // maxCharsPerLine and maxLines
  if (changed)
    computeFontMetrics_();
}
void PocketmageEink::einkTextDynamic(bool doFull, bool noRefresh) {
  if (!lines_ || !currentFont_ || !dynamicScroll_)
    return;

  // SET FONT
  setTXTFont(currentFont_);

  // ITERATE AND DISPLAY
  uint8_t size = lines_->size();
  uint8_t displayLines = maxLines_;

  if (displayLines > size)
    displayLines = size;

  int scrollOffset = *dynamicScroll_;
  if (scrollOffset < 0)
    scrollOffset = 0;
  if (scrollOffset > size - displayLines)
    scrollOffset = size - displayLines;

  if (doFull) {
    display_.fillScreen(GxEPD_WHITE);
    for (uint8_t i = size - displayLines - scrollOffset; i < size - scrollOffset; i++) {
      if ((*lines_)[i].length() == 0)
        continue;
      display_.setFullWindow();
      // display_.fillRect(0, (fontHeight_ + lineSpacing_) * (i - (size - displayLines -
      // scrollOffset)), display.width(), (fontHeight_ + lineSpacing_), GxEPD_WHITE)
      display_.setCursor(0, fontHeight_ + ((fontHeight_ + lineSpacing_) *
                                           (i - (size - displayLines - scrollOffset))));
      display_.print((*lines_)[i]);
    }
  }
  // PARTIAL REFRESH, ONLY SEND LAST LINE
  else {
    if ((*lines_)[size - displayLines - scrollOffset].length() > 0) {
      int y = (fontHeight_ + lineSpacing_) * (size - displayLines - scrollOffset);
      display_.setPartialWindow(0, y, display_.width(), (fontHeight_ + lineSpacing_));
      display_.fillRect(0, y, display_.width(), (fontHeight_ + lineSpacing_), GxEPD_WHITE);
      display_.setCursor(0, fontHeight_ + y);
      display_.print((*lines_)[size - displayLines - scrollOffset]);
    }
  }

  drawStatusBar(String("L:") + String(lines_->size()) + " " + *editingFile_);
}
int PocketmageEink::countLines(const String& input, size_t maxLineLength) {
  size_t inputLength = input.length();
  uint8_t charCounter = 0;
  uint16_t lineCounter = 1;
  for (size_t c = 0; c < inputLength; c++) {
    if (input[c] == '\n') {
      charCounter = 0;
      lineCounter++;
      continue;
    }
    if (charCounter > (maxLineLength - 1)) {
      charCounter = 0;
      lineCounter++;
    }
    charCounter++;
  }

  return lineCounter;
}
void PocketmageEink::forceSlowFullUpdate(bool force) {
  forceSlowFullUpdate_ = force;
}

// ===================== getter functions =====================
const GFXfont* PocketmageEink::getCurrentFont() {
  return currentFont_;
}
uint8_t PocketmageEink::maxCharsPerLine() const {
  return maxCharsPerLine_;
}
uint8_t PocketmageEink::maxLines() const {
  return maxLines_;
}
uint8_t PocketmageEink::getFontHeight() const {
  return fontHeight_;
}
uint8_t PocketmageEink::getLineSpacing() const {
  return lineSpacing_;
}
DisplayT& PocketmageEink::getDisplay() const {
  return display_;
}