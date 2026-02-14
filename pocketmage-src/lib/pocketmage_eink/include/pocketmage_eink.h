//  oooooooooooo         ooooo ooooo      ooo oooo    oooo  //
//  `888'     `8         `888' `888b.     `8' `888   .8P'   //
//   888                  888   8 `88b.    8   888  d8'     //
//   888oooo8    8888888  888   8   `88b.  8   88888[       //
//   888    "             888   8     `88b.8   888`88b.     //
//   888       o          888   8       `888   888  `88b.   //
//  o888ooooood8         o888o o8o        `8  o888o  o888o  //

#pragma once
#include <Arduino.h>
#include <GxEPD2_BW.h>
#include <config.h>  // for FULL_REFRESH_AFTER

#include <vector>
#pragma region fonts
// FONTS
// 3x7
#include "Fonts/Font3x7FixedNum.h"
#include "Fonts/Font4x5Fixed.h"
#include "Fonts/Font5x7Fixed.h"

// 9x7
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSerif9pt7b.h>
#include <Fonts/FreeSerifBold9pt7b.h>
// 12x7
#include <Fonts/FreeMono12pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSerif12pt7b.h>
// Font includes
// Mono
#include <Fonts/FreeMono9pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeMonoBold18pt7b.h>
#include <Fonts/FreeMonoBold24pt7b.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeMonoBoldOblique12pt7b.h>
#include <Fonts/FreeMonoBoldOblique18pt7b.h>
#include <Fonts/FreeMonoBoldOblique24pt7b.h>
#include <Fonts/FreeMonoBoldOblique9pt7b.h>
#include <Fonts/FreeMonoOblique9pt7b.h>

// Serif
#include <Fonts/FreeSerif9pt7b.h>
#include <Fonts/FreeSerifBold12pt7b.h>
#include <Fonts/FreeSerifBold18pt7b.h>
#include <Fonts/FreeSerifBold24pt7b.h>
#include <Fonts/FreeSerifBold9pt7b.h>
#include <Fonts/FreeSerifBoldItalic12pt7b.h>
#include <Fonts/FreeSerifBoldItalic18pt7b.h>
#include <Fonts/FreeSerifBoldItalic24pt7b.h>
#include <Fonts/FreeSerifBoldItalic9pt7b.h>
#include <Fonts/FreeSerifItalic9pt7b.h>

// Sans
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSansBold24pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSansBoldOblique12pt7b.h>
#include <Fonts/FreeSansBoldOblique18pt7b.h>
#include <Fonts/FreeSansBoldOblique24pt7b.h>
#include <Fonts/FreeSansBoldOblique9pt7b.h>
#include <Fonts/FreeSansOblique9pt7b.h>
#pragma endregion

// Type alias for readability
using PanelT = GxEPD2_310_GDEQ031T10;
using DisplayT = GxEPD2_BW<PanelT, PanelT::HEIGHT>;

// ===================== EINK CLASS =====================
class PocketmageEink {
 public:
  explicit PocketmageEink(DisplayT& display);

  // Wire up external buffers/state used to read from globals
  void setTextBuffer(std::vector<String>* lines) {
    lines_ = lines;
  };  // reference to allLines
  void setEditingFilePtr(String* editingFile) {
    editingFile_ = editingFile;
  };  // reference to editingFile string
  void setDynamicScroll(volatile long* dynamicScroll) {
    dynamicScroll_ = dynamicScroll;
  };  // reference to dynamicScroll
  void setLineSpacing(uint8_t lineSpacing) {
    lineSpacing_ = lineSpacing;
  };  // reference to lineSpacing (default 6)
  void setFullRefreshAfter(uint8_t fullRefreshAfter) {
    fullRefreshAfter_ = fullRefreshAfter;
  };  // reference to FULL_REFRESH_AFTER (default 5)
  void setCurrentFont(const GFXfont* font) {
    if (currentFont_ == font)
      return;
    currentFont_ = font;
  };  // reference to currentFont

  // Main display functions
  void refresh();
  void multiPassRefresh(int passes);
  void setFastFullRefresh(bool setting);
  void statusBar(const String& input, bool fullWindow = false);
  void drawStatusBar(const String& input);
  void computeFontMetrics_();
  void setTXTFont(const GFXfont* font);
  void einkTextDynamic(bool doFull, bool noRefresh = false);
  int countLines(const String& input, size_t maxLineLength = 29);

  // getters
  uint8_t maxCharsPerLine() const;
  uint8_t maxLines() const;
  const GFXfont* getCurrentFont();
  uint8_t getFontHeight() const;
  DisplayT& getDisplay() const;
  uint8_t getLineSpacing() const;

  void forceSlowFullUpdate(bool force);

 private:
  DisplayT& display_;  // class reference to hardware display object
  bool forceSlowFullUpdate_ = false;
  uint8_t partialCounter_ = 0;
  const GFXfont* currentFont_ = nullptr;
  uint8_t fullRefreshAfter_ = FULL_REFRESH_AFTER;

  std::vector<String>* lines_ = nullptr;
  String* editingFile_ = nullptr;

  volatile long* dynamicScroll_ = nullptr;

  // font metrics
  uint8_t lineSpacing_ = 6;
  uint8_t maxCharsPerLine_ = 0;
  uint8_t maxLines_ = 0;
  uint8_t fontHeight_ = 0;
};

void wireEink();
void setupEink();
PocketmageEink& EINK();