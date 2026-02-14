//  88888888b  888888ba    .d888888  8888ba.88ba   88888888b .d88888b      //
//  88         88    `8b  d8'    88  88  `8b  `8b  88        88.    "'     //
//  a88aaaa    a88aaaa8P' 88aaaaa88  88   88   88  88aaaa    `Y88888b.     //
//  88         88   `8b.  88     88  88   88   88  88              `8b     //
//  88         88     88  88     88  88   88   88  88        d8'   .8P     //
//  dP         dP     dP  88     88  dP   dP   dP  88888888P  Y88888P      //

#include <pocketmage.h>

// !! Commented for code-review
// to-do
// replace all frame bools with 8 bit flag 1<<0 = cursor, 1<<1 = box, 1<<2 = overlap, 1<<3 = invert,
// 1<<4 = choice? ensure max memory efficiency add frame thickness customization with
// drawThickLine() add tiling functions
// - add tracking variables for new positions (have an original position that a developer can shrink
// back to ) define tiling constants (tab left, tab right, tab top, tab bottom)
/*
Frames Class:
@Description
  This is a class that can display custom frames with various user-defined attributes

  Notes:
    MAX_FRAMES = 100, avoid overflowing to avoid reallocations
    frames displayed will clear parts of overlapping frames
    if choice != 0, frames will display an arrow to the right of the current scroll // NOTE: global
frameSelection be read and set to 0 after updateScrollFromTouch_Frames to read a frame's choice if
box = 1, a thin frame will be drawn around the box if invert = 1, text will be displayed in dark
mode if overlap = 1, frame with cover any contect behind frame frames uses a TextSource as a source
for drawn text, which can be defined as either a constant array of char, a FixedArenaSource for
dynamic text content, or a bitmap for images
    - an example of using frames can be seen in calc.cpp

  Setup:
    each frame has bounds defined by the margin of pixels from the screen's edges left,right,top,
and bottom frames point to 3 different types of sources: a fixed arena source (dynamic content), a
progremmem table (static text), and a bitmap (static image)

  Usage:
    einkFramesDynamic(std::vector<Frame*> &frames, bool doFull_) // draws all frames in
std::vector<Frame *> frames updateScroll(Frame *currentFrameState,int prevScroll,int currentScroll,
bool reset) // updates individual frame's scroll from scrollDynamic and prevScrollDynamic

    frames.clear(); // remove all frames stored in std::vector<frame *> frames
    frames.push_back(frame *); // add the frames you want to draw NOTE: frames pushed back earlier
will be drawn over if new frames have overlap set to true CurrentFrameState = &frame; // point to
current frame you want to control, can switch at any point to control different frames

    calcLines.pushLine(s.c_str(), (uint16_t)s.length(), flag); // push line to a dynamic text
source, flag currently not implemented

    std::vector<String> sourceToVector(const TextSource* src); // export frame text source to
std::vector<String> for compatibility
*/

#pragma region helpers
///////////////////////////// HELPER FUNCTIONS
// align numbers to be divisible by 8, useful for partial windows (y & h must be divisible by 8 with
// GxEPD2 with rotation == 3)
int alignDown8(int v) {
  return v - (v % 8);
}
int alignUp8(int v) {
  return (v % 8) ? v + (8 - (v % 8)) : v;
}

size_t sliceThatFits(const char* s, size_t n, int maxTextWidth) {
  if (!s || n == 0)
    return 0;

  int16_t x1, y1;
  uint16_t w, h;
  static char buf[256];
  const size_t cap = sizeof(buf) - 1;

  size_t best = 0, lastSpace = SIZE_MAX;
  size_t i = 0;
  size_t len = 0;

  while (i < n && len < cap) {
    char c = s[i];

    // newline: either end before it, or consume 1 char if it's first
    if (c == '\n' || c == '\r')
      return (best > 0) ? best : 1;

    if (c == ' ')
      lastSpace = i;

    buf[len++] = c;
    buf[len] = '\0';

    EINK().getDisplay().getTextBounds(buf, 0, 0, &x1, &y1, &w, &h);
    if ((int)w > maxTextWidth)
      break;

    best = i + 1;
    ++i;
  }

  const bool overflowed = (i < n) || (len >= cap);
  if (best == 0)
    return (n ? 1 : 0);

  if (overflowed && lastSpace != SIZE_MAX && lastSpace + 1 <= best) {
    return lastSpace + 1;
  }
  return best;
}
// GET TOTAL LINES OF SOURCE !!
inline long totalLines(const Frame& frame) {
  return frame.source ? (long)frame.source->size() : 0L;
}
// FIND MAX SCROLL BASED ON TXT SOURCE SIZE AND FRAME'S MAX LINES !!
long maxScroll(const Frame& frame) {
  long tl = totalLines(frame);
  long ml = (long)frame.maxLines;
  if (ml <= 0)
    return 0;
  return (tl > ml) ? (tl - ml) : 0;
}
// ENSURE SCROLL IS WITHIN BOUNDS OF VISIBLE TEXT !!
inline void clampScroll(Frame& frame) {
  long maxStart = maxScroll(frame);
  if (frame.scroll < 0)
    frame.scroll = 0;
  if (frame.scroll > maxStart)
    frame.scroll = maxStart;
}
// REMOVE CARRAIGE RETURN AND LINE FEED !!
inline size_t trimCRLF(const char* s, size_t n) {
  while (n && (s[n - 1] == '\n' || s[n - 1] == '\r'))
    --n;
  return n;
}

// MAKE SURE CHOICE IS VISIBLE IN FRAME --
void ensureChoiceVisible(Frame& frame) {
  long T = frame.source ? (long)frame.source->size() : 0L;
  int D = max(1, frame.maxLines);
  if (T == 0 || frame.choice < 0 || frame.choice >= T)
    return;

  // choose a start index so that choice is within [start, start + D)
  long start = frame.choice - (D - 1);
  if (start < 0)
    start = 0;
  long maxStart = (T > D) ? (T - D) : 0;
  if (start > maxStart)
    start = maxStart;
  frame.scroll = T - D - start;
}
// REST SCROLL OF FRAME !!
void resetScroll(Frame& frame) {
  long tl = (long)(frame.source ? frame.source->size() : 0);
  long ml = (long)frame.maxLines;
  frame.scroll = tl > ml ? (tl - ml) : 0;
  frame.prevScroll = -1;
}
// GET CLEANED STRING FROM FRAME CHOICE -- NOTE: remove ~C~ and ~R~ in refactor to lineview flages
String frameChoiceString(const Frame& f) {
  LineView lv = f.source->line(f.choice);
  String s(lv.ptr, lv.len);
  if (s.startsWith("~C~") || s.startsWith("~R~"))
    s.remove(0, 3);
  s.trim();
  return s;
}
// COPY TEXTSOURCE TO STD::VECTOER<STRING> MEMORY INEFFICIENT REMOVE IF STD::VECTOR<STRING> LINES
// ARE DEPRECIATED
std::vector<String> sourceToVector(const TextSource* src) {
  std::vector<String> result;
  if (!src)
    return result;

  result.reserve(src->size());
  for (size_t i = 0; i < src->size(); ++i) {
    LineView lv = src->line(i);
    // copy the chars into an Arduino String
    result.push_back(String(lv.ptr).substring(0, lv.len));
  }
  return result;
}
#pragma endregion

///////////////////////////// DRAWING FUNCTIONS
// DRAW ALL FRAMES STORED WITHIN TOTAL FRAME BOUNDING BOX -- NOTE: remove ~C~ and ~R~ with switch to
// lineview flags
void einkFramesDynamic(std::vector<Frame*>& frames, bool doFull_) {
  if (frames.empty())
    return;

  // compute union window of all frames
  int minX = 32767, minY = 32767;
  int maxX = -32768, maxY = -32768;
  for (Frame* frame : frames) {
    if (!frame)
      continue;
    const int width = EINK().getDisplay().width() - frame->left - frame->right;
    const int height = EINK().getDisplay().height() - frame->top - frame->bottom;
    if (width <= 0 || height <= 0)
      continue;
    if (frame->left < minX)
      minX = frame->left;
    if (frame->top < minY)
      minY = frame->top;
    if (frame->left + width > maxX)
      maxX = frame->left + width;
    if (frame->top + height > maxY)
      maxY = frame->top + height;
  }
  if (minX > maxX || minY > maxY)
    return;

  const int frameX = minX;
  const int frameY = alignDown8(minY);
  const int frameW = alignUp8(maxX - minX);
  const int frameH = alignDown8(maxY - frameY);

  EINK().setTXTFont(EINK().getCurrentFont());

  EINK().getDisplay().setPartialWindow(frameX, frameY, frameW, frameH);
  EINK().getDisplay().firstPage();
  do {
    if (doFull_) {
      EINK().getDisplay().fillRect(frameX, frameY, frameW, frameH, GxEPD_WHITE);
    }
    Serial.println("Drawing Text!");
    for (Frame* frame : frames) {
      if (!frame || (!frame->source && !frame->bitmap))
        continue;
      Serial.println("In frame loop");
      const int frameW = EINK().getDisplay().width() - frame->left - frame->right;
      const int frameH = EINK().getDisplay().height() - frame->top - frame->bottom;
      if (frameW <= 0 || frameH <= 0)
        continue;

      // if frame overlaps or is inverted, fill box with proper color
      if (frame->invert || frame->overlap) {
        Serial.println("drawing invert overlap");
        EINK().getDisplay().fillRect(frame->left, frame->top, frameW, frameH,
                                     frame->invert ? GxEPD_BLACK : GxEPD_WHITE);
      }

      if (frame->box) {
        Serial.println("drawing box!");
        if (frameW > 2 && frameH > 2) {
          drawFrameBox(frame->left + 1, frame->top + 1, frameW - 2, frameH - 2, frame->invert);
        }
      }
      if (frame->bitmap) {
        Serial.println("drawing bitmap!");
        if (frame->bitmapW <= frameW && frame->bitmapH <= frameH) {
          const uint16_t bitColor = frame->invert ? GxEPD_WHITE : GxEPD_BLACK;
          if (frame->bitmapW <= frameW && frame->bitmapH <= frameH) {
            int x = frame->left + (frameW - frame->bitmapW) / 2;
            int y = frame->top + (frameH - frame->bitmapH) / 2;
            EINK().getDisplay().drawBitmap(x, y, frame->bitmap, frame->bitmapW, frame->bitmapH,
                                           bitColor);
          }
        }

        continue;
      }
      const int lineStride = EINK().getFontHeight() + EINK().getLineSpacing();

      frame->maxLines = (lineStride > 1) ? (frameH / lineStride) - 1 : 0;
      if (frame->maxLines <= 0)
        continue;
      Serial.println("Clamping Scroll");
      const long total = frame->source ? (long)frame->source->size() : 0L;
      clampScroll(*frame);

      if (frame == CurrentFrameState && frame->choice >= 0) {
        ensureChoiceVisible(*frame);
      }
      // initialize lastTotal on first draw
      if (frame->lastTotal < 0)
        frame->lastTotal = total;

      // remember if user was pinned to bottom before we adjust
      const bool wasPinnedToBottom = (frame->scroll == 0);

      // if maxLines shrank or list shrank, clamp scroll
      clampScroll(*frame);

      // if user is at bottom, keep them there when lines grow
      if (wasPinnedToBottom)
        frame->scroll = 0;

      // update last seen count
      frame->lastTotal = total;
      long startLine = 0, endLine = 0;
      // now get the visible range with the reconciled values

      const int maxTextWidth = frameW;
      int outLine = 0;
      getVisibleRange(frame, total, startLine, endLine);
      // force the visible window to the selected line when only one line fits
      if (frame->maxLines <= 1 && frame->choice >= 0 && frame->choice < total) {
        startLine = frame->choice;
        endLine = frame->choice + 1;
      }
      for (long line = startLine; line < endLine; ++line) {
        LineView lv = frame->source->line(line);
        size_t effLen = trimCRLF(lv.ptr, lv.len);
        if (effLen == 0) {
          ++outLine;
          continue;
        }

        const bool right = (lv.flags & LF_RIGHT) != 0;
        const bool center = (lv.flags & LF_CENTER) != 0;

        const bool isSelectedLine = (frame == CurrentFrameState) && (frame->choice == line);
        bool firstSlice = true;

        size_t pos = 0;
        while (pos < effLen) {
          size_t take = sliceThatFits(lv.ptr + pos, effLen - pos, maxTextWidth);
          if (take == 0)
            break;

          String toPrint;
          if (right)
            toPrint = "~R~";
          else if (center)
            toPrint = "~C~";

          toPrint.concat(String(lv.ptr + pos).substring(0, take));

          if (isSelectedLine && firstSlice) {
            toPrint = toPrint + "<";
          }
          // draw with the current visual row index
          drawLineInFrame(toPrint, outLine++, *frame, 0, false, !doFull_);

          pos += take;
          firstSlice = false;
        }
      }
    }
    // Serial.println("Stopped Drawing Text!");
  } while (EINK().getDisplay().nextPage());
  // Serial.println("Done drawing frames!");
}
// DRAW BOX AROUND FRAME !!
void drawFrameBox(int usableX, int usableY, int usableWidth, int usableHeight, bool invert) {
  // draw box around frame within the partial window
  if (invert) {
    EINK().getDisplay().drawFastHLine(usableX, usableY, usableWidth, GxEPD_WHITE);  // Top
    EINK().getDisplay().drawFastHLine(usableX, usableY + usableHeight - 1, usableWidth,
                                      GxEPD_WHITE);                                  // Bottom
    EINK().getDisplay().drawFastVLine(usableX, usableY, usableHeight, GxEPD_WHITE);  // Left
    EINK().getDisplay().drawFastVLine(usableX + usableWidth - 1, usableY, usableHeight,
                                      GxEPD_WHITE);  // Right
  } else {
    EINK().getDisplay().drawFastHLine(usableX, usableY, usableWidth, GxEPD_BLACK);  // Top
    EINK().getDisplay().drawFastHLine(usableX, usableY + usableHeight - 1, usableWidth,
                                      GxEPD_BLACK);                                  // Bottom
    EINK().getDisplay().drawFastVLine(usableX, usableY, usableHeight, GxEPD_BLACK);  // Left
    EINK().getDisplay().drawFastVLine(usableX + usableWidth - 1, usableY, usableHeight,
                                      GxEPD_BLACK);  // Right
  }
}
// DRAW SINGLE LINE IN FRAME -- NOTE: remove ~C~ and ~R~ with switch to lineview flags
void drawLineInFrame(String& srcLine, int lineIndex, Frame& frame, int usableY, bool clearLine,
                     bool isPartial) {
  if (srcLine.length() == 0)
    return;
  EINK().setTXTFont(EINK().getCurrentFont());
  // get alignment and remove alignment marker
  String line = srcLine;
  bool rightAlign = line.startsWith("~R~");
  bool centerAlign = line.startsWith("~C~");
  if (rightAlign || centerAlign)
    line.remove(0, 3);
  int16_t x1, y1;
  uint16_t lineWidth, lineHeight;
  EINK().getDisplay().getTextBounds(line, 0, 0, &x1, &y1, &lineWidth, &lineHeight);
  int cursorX = computeCursorX(frame, rightAlign, centerAlign, x1, lineWidth);
  // set yRaw to frame top + spaces taken by all previous lines
  int yRaw = frame.top + lineIndex * (EINK().getFontHeight() + EINK().getLineSpacing());
  // set the cursor y so that the top of the font does not get cut off by the top of the frame
  int yDraw = yRaw + EINK().getFontHeight() - y1 / 2;
  // if clear line, clear box the size of the frame at the current line
  if (clearLine) {
    int yClear = alignDown8(yRaw);
    int clearHeight = alignUp8(EINK().getFontHeight() + EINK().getLineSpacing() + abs(y1));
    EINK().getDisplay().fillRect(frame.left, yClear,
                                 EINK().getDisplay().width() - frame.left - frame.right,
                                 clearHeight, frame.invert ? GxEPD_BLACK : GxEPD_WHITE);
  }
  EINK().getDisplay().setCursor(cursorX, yDraw);
  frame.invert ? EINK().getDisplay().setTextColor(GxEPD_WHITE)
               : EINK().getDisplay().setTextColor(GxEPD_BLACK);
  EINK().getDisplay().print(line);
}

///////////////////////////// FRAME SCROLL FUNCTIONS
// NOTE: frameSelection must be set to 0 after updating choices in corresponding app to continue
// with choice selection
void updateScrollFromTouch_Frame() {
  uint16_t touched = cap.touched();
  int newTouch = -1;
  for (int i = 0; i < 3; i++) {
    if (touched & (1 << i)) {
      newTouch = i;
      Serial.print("Prev pad: ");
      Serial.print(lastTouch);
      Serial.print("   Touched pad: ");
      Serial.println(newTouch);
      break;
    }
  }
  unsigned long currentTime = millis();
  if (newTouch != -1) {
    if (lastTouch != -1) {
      int touchDelta = abs(newTouch - lastTouch);
      if (touchDelta < 2) {
        long total = CurrentFrameState->source ? (long)CurrentFrameState->source->size() - 1 : 0L;
        if (CurrentFrameState->choice == -1) {
          // Serial.println("Adjusting scroll to clamp non-choice frames");
          total -= CurrentFrameState->maxLines + 1;
        }
        const long maxScroll = max(0L, total);
        if (newTouch > lastTouch) {
          dynamicScroll = min((long)(dynamicScroll + 1), maxScroll);
        } else if (newTouch < lastTouch) {
          dynamicScroll = max((long)(dynamicScroll - 1), 0L);
        }
        updateScroll(CurrentFrameState, prev_dynamicScroll, dynamicScroll);
        // Serial.println("updating scroll to: " + String(dynamicScroll));
        // Serial.println("max scroll is = " + String((int)total));
        if (CurrentFrameState->choice != -1) {
          CurrentFrameState->choice = CurrentFrameState->scroll;
        }
      }
    }
    lastTouch = newTouch;
    lastTouchTime = currentTime;
  } else if (lastTouch != -1) {
    if (currentTime - lastTouchTime > TOUCH_TIMEOUT_MS) {
      lastTouch = -1;
      if (prev_dynamicScroll != dynamicScroll) {
        newLineAdded = true;
        prev_dynamicScroll = dynamicScroll;
        updateScroll(CurrentFrameState, prev_dynamicScroll, dynamicScroll);
        // choice specific behavior to be defined by user, must set frameSelection to 0 once
        // addressed in app
        if (!frameSelection && CurrentFrameState->choice != -1) {
          frameSelection = 1;
        }
      }
    }
  }
}
// UPDATE CURRENT FRAME SCROLL
void updateScroll(Frame* currentFrameState, int prevScroll, int currentScroll, bool reset) {
  int scroll, prev;
  if (reset) {
    resetScroll(*currentFrameState);
  } else {
    currentFrameState->scroll = currentScroll;
    currentFrameState->prevScroll = prevScroll;
  }
  return;
}

///////////////////////////// FRAME OLED FUNCTIONS
void oledScrollFrame() {
  // CLEAR DISPLAY
  u8g2.clearBuffer();
  // draw background
  if (CurrentFrameState->choice == -1)
    u8g2.drawXBMP(0, 0, 128, 32, scrolloled0);

  // draw lines preview
  long int count = CurrentFrameState->source->size();
  long startIndex, endIndex;
  getVisibleRange(CurrentFrameState, count, startIndex, endIndex);

  // decide how many preview lines to show
  long previewTop = startIndex;
  long previewBottom = endIndex - 1;
  const int rowStep = 4;
  const int baseY = 28;

  for (long int i = previewBottom; i >= previewTop && i >= 0; --i) {
    if (i < 0 || i >= count)
      continue;
    int16_t x1, y1;
    uint16_t charWidth, charHeight;
    LineView lv = CurrentFrameState->source->line(i);
    String line = String(lv.ptr).substring(0, lv.len);

    if (line.startsWith("    ")) {
      EINK().getDisplay().getTextBounds(line.substring(4), 0, 0, &x1, &y1, &charWidth, &charHeight);
      int lineWidth = map(charWidth, 0, 320, 0, 49);
      lineWidth = constrain(lineWidth, 0, 49);

      // compute Y based on distance from newest visible line
      long posFromBottom = previewBottom - i;
      int boxY = baseY - (rowStep * posFromBottom);
      if (boxY >= 0) {
        // u8g2.drawBox(68, boxY, lineWidth, 2);
      }
    } else {
      EINK().getDisplay().getTextBounds(line, 0, 0, &x1, &y1, &charWidth, &charHeight);
      int lineWidth = map(charWidth, 0, 320, 0, 56);
      lineWidth = constrain(lineWidth, 0, 56);

      long posFromBottom = previewBottom - i;
      int boxY = baseY - (rowStep * posFromBottom);
      if (boxY >= 0 && CurrentFrameState->choice == -1) {
        u8g2.drawBox(61, boxY, lineWidth, 2);
      }
    }
  }

  long displayedLinesStart = startIndex + 1;
  long displayedLinesEnd = endIndex;
  if (count == 0) {
    displayedLinesStart = 0;
    displayedLinesEnd = 0;
  }

  if (CurrentFrameState->choice != -1) {
    //  fetch line by scroll
    long idx = CurrentFrameState->scroll;
    if (idx >= 0 && idx < count) {
      LineView plv = CurrentFrameState->source->line(idx);
      String pLine = String(plv.ptr).substring(0, plv.len);

      if (pLine.length() > 0) {
        u8g2.setFont(u8g2_font_ncenB10_tr);
        u8g2.drawStr((u8g2.getWidth() - u8g2.getUTF8Width(pLine.substring(3).c_str())) / 2, 24,
                     pLine.substring(3).c_str());
      }
    }
  } else {
    // print current line
    u8g2.setFont(u8g2_font_ncenB08_tr);
    String lineNumStr = String(count - CurrentFrameState->scroll) + "/" + String(count);
    u8g2.drawStr(0, 12, "Lines:");
    u8g2.drawStr(0, 24, lineNumStr.c_str());
  }
  // send buffer
  u8g2.sendBuffer();
}

///////////////////////////// TEXT POSITION FUNCTIONS
// FIND START AND END LINES OF FRAME !!
void getVisibleRange(Frame* f, long totalLines, long& startLine, long& endLine) {
  if (totalLines <= 0) {
    startLine = endLine = 0;
    return;
  }
  long displayLines = constrain((long)f->maxLines, 0L, totalLines);
  long scrollOffset = constrain((long)f->scroll, 0L, max(0L, totalLines - displayLines));
  // bottom scroll = 0
  startLine = max(0L, totalLines - displayLines - scrollOffset);
  endLine = min(totalLines, startLine + displayLines);
}
// COMPUTE X POS IN FRAME !!
int computeCursorX(Frame& frame, bool rightAlign, bool centerAlign, int16_t x1,
                   uint16_t lineWidth) {
  // right padding to avoid overlaps with frame
  const int padding = 16;
  int usableWidth = EINK().getDisplay().width() - frame.left - frame.right;
  int base;
  // draw lines with alignment
  if (rightAlign) {
    base = frame.left + usableWidth - lineWidth - padding;
  } else if (centerAlign) {
    base = frame.left + (usableWidth - lineWidth) / 2 - padding / 2;
  } else {
    base = frame.left;
  }
  // base - left margin + offset
  return base - x1 + X_OFFSET;
}
