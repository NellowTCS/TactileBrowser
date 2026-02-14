// dP     dP  88888888b dP    dP  888888ba   .88888.   .d888888   888888ba  888888ba  //
// 88   .d8'  88        Y8.  .8P  88    `8b d8'   `8b d8'    88   88    `8b 88    `8b //
// 88aaa8P'  a88aaaa     Y8aa8P  a88aaaa8P' 88     88 88aaaaa88a a88aaaa8P' 88     88 //
// 88   `8b.  88           88     88   `8b. 88     88 88     88   88   `8b. 88     88 //
// 88     88  88           88     88    .88 Y8.   .8P 88     88   88     88 88    .8P //
// dP     dP  88888888P    dP     88888888P  `8888P'  88     88   dP     dP 8888888P  //

#include <Adafruit_TCA8418.h>
#include <pocketmage_kb.h>
#pragma region keymaps
// ===================== Keymaps =====================
char currentKB[4][10];  // Current keyboard layout

char keysArray[4][10] = {
    {'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p'},
    {'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', 8},  // 8:BKSP
    {9, 'z', 'x', 'c', 'v', 'b', 'n', 'm', '.', 13},   // 9:TAB, 13:CR
    {0, 17, 18, ' ', ' ', ' ', 19, 20, 21, 0}          // 17:SHFT, 18:FN, 19:<-, 20:SEL, 21:->
};
char keysArraySHFT[4][10] = {{'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P'},
                             {'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', 8},
                             {9, 'Z', 'X', 'C', 'V', 'B', 'N', 'M', ',', 13},
                             {0, 17, 18, ' ', ' ', ' ', 28, 29, 30, 0}};
char keysArrayFN[4][10] = {{'1', '2', '3', '4', '5', '6', '7', '8', '9', '0'},
                           {'#', '!', '$', ':', ';', '(', ')', '\'', '\"', 8},
                           {14, '%', '_', '&', '+', '-', '/', '?', ',', 13},
                           {0, 17, 18, ' ', ' ', ' ', 12, 7, 6, 0}};
#pragma endregion

// ===================== public functions =====================
char PocketmageKB::updateKeypress() {
  if ((TCA8418_event_) && (*TCA8418_event_ == true)) {
    int k = keypad_.getEvent();

    //  try to clear the IRQ flag
    //  if there are pending events it is not cleared
    keypad_.writeRegister(TCA8418_REG_INT_STAT, 1);
    int intstat = keypad_.readRegister(TCA8418_REG_INT_STAT);
    if ((intstat & 0x01) == 0)
      *TCA8418_event_ = false;

    if (k & 0x80) {  // Key pressed, not released
      k &= 0x7F;
      k--;
      // return currentKB[k/10][k%10];
      if ((k / 10) < 4) {
        // Key was pressed, reset timeout counter
        if (prevTimeMillis_)
          *prevTimeMillis_ = millis();

        // Return Key
        switch (currentKbState()) {
          case 0:
            return keysArray[k / 10][k % 10];
          case 1:
            return keysArraySHFT[k / 10][k % 10];
          case 2:
            return keysArrayFN[k / 10][k % 10];
          default:
            return 0;
        }
      }
    }
  }

  return 0;
}

// ===================== private functions =====================
int PocketmageKB::currentKbState() const {
  if (kbStateFn_)
    return kbStateFn_();
  if (kbState_)
    return *kbState_;
  return 0;
}

// ===================== ISR =====================
// Interrupt handler stored in IRAM for fast interrupt response
void IRAM_ATTR PocketmageKB::TCA8418_irq() {
  if (TCA8418_event_)
    *TCA8418_event_ = true;
}