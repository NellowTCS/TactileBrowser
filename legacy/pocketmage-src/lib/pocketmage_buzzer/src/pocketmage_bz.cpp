//  888888ba  dP     dP d8888888P d8888888P  88888888b  888888ba  //
//  88    `8b 88     88      .d8'      .d8'  88         88    `8b //
// a88aaaa8P' 88     88    .d8'      .d8'   a88aaaa    a88aaaa8P' //
//  88   `8b. 88     88  .d8'      .d8'      88         88   `8b. //
//  88    .88 Y8.   .8P d8'       d8'        88         88     88 //
//  88888888P `Y88888P' Y8888888P Y8888888P  88888888P  dP     dP //

#include <pocketmage_bz.h>

// ===================== main functions =====================
void PocketmageBZ::playJingle(const Jingle& jingle) const {
  if (jingle.notes == nullptr || jingle.len == 0) {
    return;  // No valid notes to play
  }

  buzzer_.begin(0);  // Initialize buzzer

  for (size_t i = 0; i < jingle.len; ++i) {
    buzzer_.sound(jingle.notes[i].key, jingle.notes[i].duration);
  }

  buzzer_.sound(0, 80);  // End the sound
  buzzer_.end(0);        // Stop the buzzer
}
