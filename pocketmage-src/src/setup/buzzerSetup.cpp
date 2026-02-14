#include <pocketmage.h>

// Initialization of bz class
static PocketmageBZ pm_bz(buzzer);

// Setup for Buzzer Class
void setupBZ() {
  BZ().playJingle(Jingles::Startup);
}

// Wire function  for Buzzer class
// add any global references here + add set function to class header file
void wireBZ() {}

// Access for other apps
PocketmageBZ& BZ() {
  return pm_bz;
}
