//    .oooooo.   ooooo        oooooooooooo oooooooooo.    //
//   d8P'  `Y8b  `888'        `888'     `8 `888'   `Y8b   //
//  888      888  888          888          888      888  //
//  888      888  888          888oooo8     888      888  //
//  888      888  888          888    "     888      888  //
//  `88b    d88'  888       o  888       o  888     d88'  //
//   `Y8bood8P'  o888ooooood8 o888ooooood8 o888bood8P'    //
#include <GxEPD2_BW.h>
#include <RTClib.h>
#include <U8g2lib.h>
#include <pocketmage.h>

// Initialization of oled display class
static PocketmageOled pm_oled(u8g2);

constexpr int kBattIconCount = sizeof(batt_allArray) / sizeof(batt_allArray[0]);

// e-ink text width wrapper to work with Eink display
static uint16_t einkMeasureWidth(const String& s) {
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(s, 0, 0, &x1, &y1, &w, &h);
  return w;
}

// Setup for Oled Class
void setupOled() {
  u8g2.begin();
  u8g2.setBusClock(10000000);
  u8g2.setPowerSave(0);
  u8g2.clearBuffer();
  u8g2.sendBuffer();
  wireOled();

  // SHOW "PocketMage" while DEVICE BOOTS
  OLED().oledWord("   PocketMage   ", true, false);
}

// Wire function  for Oled class
// add any global references here + add set function to class header file
void wireOled() {
  pm_oled.setAllLines(&allLines);
  pm_oled.setDynamicScroll(&dynamicScroll);
  pm_oled.setBattery(&battState, batt_allArray, kBattIconCount);
  // lamda to avoid redundant functions To Do: make class interface for each pocketmage component in
  // library
  pm_oled.setKeyboardStateGetter([] { return static_cast<int>(CurrentKBState); });
  pm_oled.setMSC(&mscEnabled);
  pm_oled.setSD(&SDActive);
  pm_oled.setScrollBitmap(scrolloled0);
  pm_oled.setReferenceWidth(display.width());
  pm_oled.setMeasureTextWidth(einkMeasureWidth);
  pm_oled.setMaxCharsPerLineEinkGetter([] { return EINK().maxCharsPerLine(); });
  pm_oled.setClock(&CLOCK().getRTC(), &SYSTEM_CLOCK, &SHOW_YEAR, daysOfTheWeek);
}

// oled object reference for other apps
PocketmageOled& OLED() {
  return pm_oled;
}
