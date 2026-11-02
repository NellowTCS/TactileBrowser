#ifndef ASSETS_H
#define ASSETS_H

#include <globals.h>

//
extern const unsigned char KBStatusKBStatus0[] PROGMEM;
extern const unsigned char KBStatusKBStatus1[] PROGMEM;
extern const unsigned char KBStatusKBStatus2[] PROGMEM;
extern const unsigned char KBStatusKBStatus3[] PROGMEM;
extern const unsigned char KBStatusKBStatus4[] PROGMEM;
extern const unsigned char KBStatusKBStatus5[] PROGMEM;
extern const unsigned char KBStatusKBStatus6[] PROGMEM;
extern const unsigned char KBStatusKBStatus7[] PROGMEM;
extern const int KBStatusallArray_LEN;
extern const unsigned char* KBStatusallArray[8];

//
extern const unsigned char backgroundaero[] PROGMEM;
extern const unsigned char backgroundbliss[] PROGMEM;
extern const int backgroundallArray_LEN;
extern const unsigned char* backgroundallArray[2];

//
extern const unsigned char textApp[] PROGMEM;

//
extern const unsigned char _homeIcons2[] PROGMEM;
extern const unsigned char _homeIcons3[] PROGMEM;
extern const unsigned char _homeIcons4[] PROGMEM;
extern const unsigned char _homeIcons5[] PROGMEM;
extern const unsigned char _homeIcons6[] PROGMEM;
extern const unsigned char _homeIcons7[] PROGMEM;
extern const unsigned char _homeIcons8[] PROGMEM;
extern const unsigned char _homeIcons9[] PROGMEM;
extern const unsigned char _homeIcons10[] PROGMEM;
extern const unsigned char _homeIcons11[] PROGMEM;
extern const unsigned char* homeIconsAllArray[10];

extern const unsigned char _noIconFound[] PROGMEM;

//
extern const unsigned char fileWizardfileWiz0[] PROGMEM;
extern const unsigned char fileWizardfileWiz1[] PROGMEM;
extern const unsigned char fileWizardfileWiz2[] PROGMEM;
extern const unsigned char fileWizardfileWiz3[] PROGMEM;
extern const int fileWizardallArray_LEN;
extern const unsigned char* fileWizardallArray[4];

//
extern const unsigned char fileWizLitefileWizLite0[] PROGMEM;
extern const unsigned char fileWizLitefileWizLite1[] PROGMEM;
extern const unsigned char fileWizLitefileWizLite2[] PROGMEM;
extern const unsigned char fileWizLitefileWizLite3[] PROGMEM;
extern const int fileWizLiteallArray_LEN;
extern const unsigned char* fileWizLiteallArray[4];

//
extern const unsigned char sleep0[] PROGMEM;
extern const unsigned char sleep1[] PROGMEM;

//
extern const unsigned char nowLaternowAndLater0[] PROGMEM;
extern const unsigned char nowLaternowAndLater1[] PROGMEM;
extern const unsigned char nowLaternowAndLater2[] PROGMEM;
extern const unsigned char nowLaternowAndLater3[] PROGMEM;
extern const int nowLaterallArray_LEN;
extern const unsigned char* nowLaterallArray[4];

//
extern const unsigned char tasksApp0[] PROGMEM;
extern const unsigned char tasksApp1[] PROGMEM;

//
extern const unsigned char taskIconTasks0[] PROGMEM;

//
extern const unsigned char fontfont0[] PROGMEM;

//
extern const unsigned char scrolloled0[] PROGMEM;

//
extern const unsigned char _ScreenSaver0[] PROGMEM;
extern const unsigned char _ScreenSaver1[] PROGMEM;
extern const unsigned char _ScreenSaver2[] PROGMEM;
extern const unsigned char _ScreenSaver3[] PROGMEM;
extern const unsigned char _ScreenSaver4[] PROGMEM;
extern const unsigned char _ScreenSaver5[] PROGMEM;
extern const unsigned char _ScreenSaver6[] PROGMEM;
extern const unsigned char _ScreenSaver7[] PROGMEM;
extern const unsigned char _ScreenSaver8[] PROGMEM;
extern const unsigned char _ScreenSaver9[] PROGMEM;
extern const unsigned char _ScreenSaver10[] PROGMEM;
extern const unsigned char _ScreenSaver11[] PROGMEM;
extern const unsigned char _ScreenSaver12[] PROGMEM;
extern const unsigned char _ScreenSaver13[] PROGMEM;
extern const unsigned char _ScreenSaver14[] PROGMEM;
extern const unsigned char _ScreenSaver15[] PROGMEM;
extern const unsigned char _ScreenSaver16[] PROGMEM;
extern const unsigned char _ScreenSaver17[] PROGMEM;
extern const unsigned char* ScreenSaver_allArray[18];

//
extern const unsigned char _batt0[] PROGMEM;
extern const unsigned char _batt1[] PROGMEM;
extern const unsigned char _batt2[] PROGMEM;
extern const unsigned char _batt3[] PROGMEM;
extern const unsigned char _batt4[] PROGMEM;
extern const unsigned char _batt5[] PROGMEM;
extern const unsigned char* batt_allArray[6];

//
extern const unsigned char _settings[] PROGMEM;

//
extern const unsigned char _toggle[] PROGMEM;
extern const unsigned char _toggleON[] PROGMEM;
extern const unsigned char _toggleOFF[] PROGMEM;

//
extern const unsigned char _calendar00[] PROGMEM;
extern const unsigned char _calendar01[] PROGMEM;
extern const unsigned char _calendar02[] PROGMEM;
extern const unsigned char _calendar03[] PROGMEM;
extern const unsigned char _calendar04[] PROGMEM;
extern const unsigned char _calendar05[] PROGMEM;
extern const unsigned char _calendar06[] PROGMEM;
extern const unsigned char _calendar07[] PROGMEM;
extern const unsigned char _calendar08[] PROGMEM;
extern const unsigned char _calendar09[] PROGMEM;
extern const unsigned char _calendar10[] PROGMEM;
extern const unsigned char* calendar_allArray[11];

//
extern const unsigned char _lex0[] PROGMEM;
extern const unsigned char _lex1[] PROGMEM;
extern const unsigned char* lex_allArray[2];

//
extern const unsigned char _usb[] PROGMEM;

//
extern const unsigned char _eventMarker0[] PROGMEM;
extern const unsigned char _eventMarker1[] PROGMEM;

//
extern const unsigned char _journal[] PROGMEM;

//
extern const unsigned char _appLoader[] PROGMEM;

// File icons
extern const unsigned char _LFileIcon0[] PROGMEM;
extern const unsigned char _LFileIcon1[] PROGMEM;
extern const unsigned char _LFileIcon2[] PROGMEM;
extern const unsigned char _LFileIcon3[] PROGMEM;
extern const unsigned char* _LFileIcons[4];

extern const unsigned char _SFileIcon0[] PROGMEM;
extern const unsigned char _SFileIcon1[] PROGMEM;
extern const unsigned char _SFileIcon2[] PROGMEM;
extern const unsigned char _SFileIcon3[] PROGMEM;
extern const unsigned char* _SFileIcons[4];

// ===================== BROWSER APP ASSETS =====================
extern const unsigned char browserApp0[] PROGMEM;
extern const int browserAllArray_LEN;
extern const unsigned char* browserAllArray[1];

#pragma region setupData
#pragma region browserHelpText

const char bh0[] PROGMEM = "\n";
const char bh1[] PROGMEM = " vvv scroll down vvv\n";
const char bh2[] PROGMEM = " PocketMage Browser Help\n";
const char bh3[] PROGMEM = "\n";
const char bh4[] PROGMEM = " press ESC to go back\n";
const char bh5[] PROGMEM = "\n";
const char bh6[] PROGMEM = " NAVIGATION:\n";
const char bh7[] PROGMEM = " ESC -> EXIT to HOME\n";
const char bh8[] PROGMEM = "\n";
const char bh9[] PROGMEM = " scroll <- ->\n";
const char bh10[] PROGMEM = "\n";
const char bh11[] PROGMEM = " BROWSER CONTROLS:\n";
const char bh12[] PROGMEM = "\n";
const char bh13[] PROGMEM = " FN -> Enter URL\n";
const char bh14[] PROGMEM = " TAB -> Tab Management\n";
const char bh15[] PROGMEM = " ESC -> Back/Exit\n";
const char bh16[] PROGMEM = "\n";
const char bh17[] PROGMEM = " URL INPUT MODE:\n";
const char bh18[] PROGMEM = " Type URL and press ENTER\n";
const char bh19[] PROGMEM = " Must start with http://\n";
const char bh20[] PROGMEM = " or https://\n";
const char bh21[] PROGMEM = " ESC cancels input\n";
const char bh22[] PROGMEM = "\n";
const char bh23[] PROGMEM = " TAB MANAGEMENT:\n";
const char bh24[] PROGMEM = " Type tab number (1-5)\n";
const char bh25[] PROGMEM = " and press ENTER\n";
const char bh26[] PROGMEM = " Select tab 6 to create\n";
const char bh27[] PROGMEM = " new tab (if space)\n";
const char bh28[] PROGMEM = " ESC returns to browser\n";
const char bh29[] PROGMEM = "\n";
const char bh30[] PROGMEM = " SUPPORTED FEATURES:\n";
const char bh31[] PROGMEM = " - HTTP/HTTPS pages\n";
const char bh32[] PROGMEM = " - Basic HTML parsing\n";
const char bh33[] PROGMEM = " - Text content display\n";
const char bh34[] PROGMEM = " - Multiple tabs (up to 5)\n";
const char bh35[] PROGMEM = " - Scrolling through content\n";
const char bh36[] PROGMEM = "\n";
const char bh37[] PROGMEM = " LIMITATIONS:\n";
const char bh38[] PROGMEM = " - Text-only display\n";
const char bh39[] PROGMEM = " - No JavaScript support\n";
const char bh40[] PROGMEM = " - No CSS styling\n";
const char bh41[] PROGMEM = " - No images or media\n";
const char bh42[] PROGMEM = " - Limited to simple sites\n";
const char bh43[] PROGMEM = "\n";
const char bh44[] PROGMEM = " ^^^ scroll up ^^^";

// Table of pointers (in PROGMEM) and count
const char* const BROWSER_HELP_LINES[] PROGMEM = {
    bh0,  bh1,  bh2,  bh3,  bh4,  bh5,  bh6,  bh7,  bh8,  bh9,  bh10, bh11, bh12, bh13, bh14,
    bh15, bh16, bh17, bh18, bh19, bh20, bh21, bh22, bh23, bh24, bh25, bh26, bh27, bh28, bh29,
    bh30, bh31, bh32, bh33, bh34, bh35, bh36, bh37, bh38, bh39, bh40, bh41, bh42, bh43, bh44};

const size_t BROWSER_HELP_COUNT = sizeof(BROWSER_HELP_LINES) / sizeof(BROWSER_HELP_LINES[0]);

#pragma endregion  // End of browserHelpText

#pragma region browserBookmarks

const char bb0[] PROGMEM = "~C~example.com\n";
const char bb1[] PROGMEM = "~C~google.com\n";
const char bb2[] PROGMEM = "~C~wikipedia.org\n";
const char bb3[] PROGMEM = "~C~github.com\n";
const char bb4[] PROGMEM = "~C~stackoverflow.com\n";
const char bb5[] PROGMEM = "~C~reddit.com\n";
const char bb6[] PROGMEM = "~C~news.ycombinator.com\n";
const char bb7[] PROGMEM = "~C~archive.org\n";
const char bb8[] PROGMEM = "~C~duckduckgo.com\n";
const char bb9[] PROGMEM = "~C~httpbin.org\n";

const char* const BROWSER_BOOKMARKS_LINES[] PROGMEM = {bb0, bb1, bb2, bb3, bb4,
                                                       bb5, bb6, bb7, bb8, bb9};

const size_t BROWSER_BOOKMARKS_COUNT =
    sizeof(BROWSER_BOOKMARKS_LINES) / sizeof(BROWSER_BOOKMARKS_LINES[0]);

#pragma endregion  // End of browserBookmarks

#pragma region browserStatusMessages

const char bs0[] PROGMEM = "Loading...\n";
const char bs1[] PROGMEM = "Connected\n";
const char bs2[] PROGMEM = "Failed to connect\n";
const char bs3[] PROGMEM = "Parsing HTML...\n";
const char bs4[] PROGMEM = "Ready\n";
const char bs5[] PROGMEM = "Error: Invalid URL\n";
const char bs6[] PROGMEM = "Error: Network timeout\n";
const char bs7[] PROGMEM = "Error: DNS failed\n";
const char bs8[] PROGMEM = "Page loaded successfully\n";

const char* const BROWSER_STATUS_LINES[] PROGMEM = {bs0, bs1, bs2, bs3, bs4, bs5, bs6, bs7, bs8};

const size_t BROWSER_STATUS_COUNT = sizeof(BROWSER_STATUS_LINES) / sizeof(BROWSER_STATUS_LINES[0]);

#pragma endregion  // End of browserStatusMessages

#pragma endregion

#endif