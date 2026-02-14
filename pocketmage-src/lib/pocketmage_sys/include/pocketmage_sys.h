#pragma once
#include <Arduino.h>

#include <vector>

class String;

extern bool rebootToPocketMage();

namespace pocketmage {
namespace file {
void saveFile();
void writeMetadata(const String& path);
void loadFile(bool showOLED = true);
void delFile(String fileName);
void deleteMetadata(String path);
void renFile(String oldFile, String newFile);
void renMetadata(String oldPath, String newPath);
void copyFile(String oldFile, String newFile);
void appendToFile(String path, String inText);
}

namespace time {
void setTimeFromString(String timeStr);
void checkTimeout();
void setCpuSpeed(int newFreq);
}
namespace power {
void deepSleep(bool alternateScreenSaver = false);
void IRAM_ATTR PWR_BTN_irq();
void updateBattState();
void loadState(bool changeState = true);
}
namespace debug {
void printDebug();
}
}

// ===================== SYSTEM SETUP =====================
void PocketMage_INIT();
// ===================== GLOBAL TEXT HELPERS =====================
String vectorToString();
void stringToVector(String inputText);
String removeChar(String str, char character);
int stringToInt(String str);
