//  oooooooooooo         ooooo ooooo      ooo oooo    oooo  //
//  `888'     `8         `888' `888b.     `8' `888   .8P'   //
//   888                  888   8 `88b.    8   888  d8'     //
//   888oooo8    8888888  888   8   `88b.  8   88888[       //
//   888    "             888   8     `88b.8   888`88b.     //
//   888       o          888   8       `888   888  `88b.   //
//  o888ooooood8         o888o o8o        `8  o888o  o888o  //
#include <pocketmage.h>
#include <GxEPD2_BW.h>

// Initialization of eink display class
static PocketmageEink pm_eink(display);

// Setup for Eink Class
void setupEink() {
  wireEink();
  display.init(115200);
  display.setRotation(3);
  display.setTextColor(GxEPD_BLACK);
  display.setFullWindow();
  EINK().setTXTFont(&FreeMonoBold9pt7b); // default font, computeFontMetrics_()

  xTaskCreatePinnedToCore(
    einkHandler,             // Function name
    "einkHandlerTask",       // Task name
    10000,                   // Stack size
    NULL,                    // Parameters 
    1,                       // Priority 
    &einkHandlerTaskHandle,  // Task handle
    1                        // Core ID 
  );

}

// Wire function  for Eink class
// add any global references here + add set function to class header file
void wireEink() {
  pm_eink.setTextBuffer(&allLines);
  pm_eink.setEditingFilePtr(&editingFile);
  pm_eink.setDynamicScroll(&dynamicScroll);
}

// Access for other apps 
PocketmageEink& EINK() { return pm_eink; }
