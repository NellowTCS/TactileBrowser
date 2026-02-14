#ifndef CONFIG_H
#define CONFIG_H

// CONFIGURATION & SETTINGS
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////|
#define KB_COOLDOWN 50        // Keypress cooldown
#define FULL_REFRESH_AFTER 5  // Full refresh after N partial refreshes (CHANGE WITH CAUTION)
#define MAX_FILES 10          // Number of files to store
#define FORMAT_SPIFFS_IF_FAILED true  // Format the SPIFFS filesystem if mount fails
#define SLEEPMODE "TEXT"              // TEXT, SPLASH, CLOCK
#define TXT_APP_STYLE 1               // 0: Old Style (NOT SUPPORTED), 1: New Style
#define SET_CLOCK_ON_UPLOAD false     // Should system clock be set automatically on code upload?
#define TOUCH_TIMEOUT_MS 1200         // Delay after scrolling to return to typing mode (ms)
#define SYS_METADATA_FILE "/sys/SDMMC_META.txt"  // File path to the file system metadata file
#define POWER_SAVE_FREQ 40                       // CPU freq for power save mode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////|

// PIN DEFINITION
#define I2C_SCL 35
#define I2C_SDA 36
#define MPR121_ADDR 0x5A

#define KB_IRQ 8
#define PWR_BTN 38
#define BAT_SENS 4
#define CHRG_SENS 39
#define RTC_INT 1

#define SPI_MOSI 14
#define SPI_SCK 15

#define OLED_CS 47
#define OLED_DC 46
#define OLED_RST 45

#define EPD_CS 2
#define EPD_DC 21
#define EPD_RST 9
#define EPD_BUSY 37

#define SD_CLK 12
#define SD_CMD 11
#define SD_D0 13

#endif