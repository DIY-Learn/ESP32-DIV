#ifndef TOOLS_MENU_H
#define TOOLS_MENU_H

#include <TFT_eSPI.h>

extern TFT_eSPI tft;
extern bool readButtonOnce(int pin);
extern void drawStatusBarBright();

// Colors (same as your main menu)
#ifndef BG
#define BG        0x0000
#define TILE      0x2104
#define BORDER    0x39C7
#define SELECT    0xFD20
#define GREEN     0x07E0
#define WHITE     0xFFFF
#define RED       0xF800
#endif

// Tool pins
#define TOOL_FAN_PIN    37
#define TOOL_LED2_PIN   38
#define TOOL_LED3_PIN   39   // Relay / vibration motor

const char* tools[] = { "Cooling Fan", "LED Strip", "Vibration Motor", "Back to Main" };
#define TOOLS_COUNT 4

int toolsSelected = 0;
bool toolStates[3] = {false, false, false};

// ------------------- Initialization -------------------
void initTools() {
  pinMode(TOOL_FAN_PIN, OUTPUT);
  pinMode(TOOL_LED2_PIN, OUTPUT);
  pinMode(TOOL_LED3_PIN, OUTPUT);
  digitalWrite(TOOL_FAN_PIN, LOW);
  digitalWrite(TOOL_LED2_PIN, LOW);
  digitalWrite(TOOL_LED3_PIN, LOW);
}

// ------------------- Set tool state -------------------
void setTool(int index, bool state) {
  if (index >= 3) return;
  toolStates[index] = state;
  switch (index) {
    case 0: digitalWrite(TOOL_FAN_PIN, state ? HIGH : LOW); break;
    case 1: digitalWrite(TOOL_LED2_PIN, state ? HIGH : LOW); break;
    case 2: digitalWrite(TOOL_LED3_PIN, state ? HIGH : LOW); break;
  }
  // Optional: print to serial for debugging
  Serial.print("Tool "); Serial.print(index); 
  Serial.print(" set to "); Serial.println(state ? "HIGH" : "LOW");
}

// ------------------- UI drawing -------------------
void drawToolsMenu() {
  tft.fillRect(0, 23, 240, 297, BG);
  drawStatusBarBright();
  tft.setTextColor(WHITE, BG);
  tft.setCursor(10, 35);
  tft.print("Tools");

  for (int i = 0; i < TOOLS_COUNT; i++) {
    int y = 55 + i * 36;
    if (i == TOOLS_COUNT - 1) y += 5;
    uint16_t color = (i == toolsSelected) ? SELECT : GREEN;
    tft.setTextColor(color, BG);

    // Icon area
    int iconX = 10;
    int iconY = y - 12;
    tft.fillRoundRect(iconX, iconY, 32, 32, 6, TILE);
    tft.drawRoundRect(iconX, iconY, 32, 32, 6, BORDER);
    if (i < 3) {
      uint16_t ledColor = toolStates[i] ? GREEN : TILE;
      tft.fillCircle(iconX + 16, iconY + 16, 8, ledColor);
      tft.drawCircle(iconX + 16, iconY + 16, 8, BORDER);
    } else {
      tft.fillTriangle(iconX+8, iconY+16, iconX+24, iconY+8, iconX+24, iconY+24, WHITE);
    }

    tft.setCursor(50, y);
    tft.print(tools[i]);
    
    if (i < 3) {
      tft.setCursor(180, y);
      tft.setTextColor(toolStates[i] ? GREEN : RED, BG);
      tft.print(toolStates[i] ? "ON " : "OFF");
    }
  }
}

void animateToggle(int toolIndex, bool newState, int yPos) {
  for (int i = 0; i < 3; i++) {
    uint16_t flashColor = (i % 2 == 0) ? (newState ? GREEN : RED) : BG;
    tft.fillRect(180, yPos - 8, 40, 20, BG);
    tft.setTextColor(flashColor, BG);
    tft.setCursor(180, yPos);
    tft.print(newState ? "ON " : "OFF");
    delay(50);
  }
  tft.fillRect(180, yPos - 8, 40, 20, BG);
  tft.setTextColor(newState ? GREEN : RED, BG);
  tft.setCursor(180, yPos);
  tft.print(newState ? "ON " : "OFF");
}

void updateSingleToolUI(int toolIndex) {
  if (toolIndex >= 3) return;
  int yPos = 55 + toolIndex * 36;
  int iconX = 10;
  int iconY = yPos - 12;
  uint16_t ledColor = toolStates[toolIndex] ? GREEN : TILE;
  tft.fillCircle(iconX + 16, iconY + 16, 8, ledColor);
  tft.drawCircle(iconX + 16, iconY + 16, 8, BORDER);
  tft.fillRect(180, yPos - 8, 40, 20, BG);
  tft.setTextColor(toolStates[toolIndex] ? GREEN : RED, BG);
  tft.setCursor(180, yPos);
  tft.print(toolStates[toolIndex] ? "ON " : "OFF");
}

// ------------------- Menu handler with long press for fan & LED -------------------
bool handleToolsMenu() {
  static unsigned long upPressStart = 0;
  static unsigned long downPressStart = 0;
  static bool upLongTriggered = false;
  static bool downLongTriggered = false;
  static bool upWasPressed = false;
  static bool downWasPressed = false;

  int old = toolsSelected;
  bool redrawSelection = false;

  // UP button (long press = toggle LED strip, tool 1)
  bool upNow = (digitalRead(BTN_UP) == LOW);
  if (upNow && !upWasPressed) {
    upPressStart = millis();
    upLongTriggered = false;
    upWasPressed = true;
  } else if (upNow && upWasPressed && !upLongTriggered) {
    if (millis() - upPressStart >= 3000) {
      bool newState = !toolStates[1];
      setTool(1, newState);
      updateSingleToolUI(1);
      upLongTriggered = true;
    }
  } else if (!upNow && upWasPressed) {
    if (!upLongTriggered && (millis() - upPressStart < 3000)) {
      toolsSelected = (toolsSelected - 1 + TOOLS_COUNT) % TOOLS_COUNT;
      redrawSelection = true;
    }
    upWasPressed = false;
    upLongTriggered = false;
  }

  // DOWN button (long press = toggle cooling fan, tool 0)
  bool downNow = (digitalRead(BTN_DOWN) == LOW);
  if (downNow && !downWasPressed) {
    downPressStart = millis();
    downLongTriggered = false;
    downWasPressed = true;
  } else if (downNow && downWasPressed && !downLongTriggered) {
    if (millis() - downPressStart >= 3000) {
      bool newState = !toolStates[0];
      setTool(0, newState);
      updateSingleToolUI(0);
      downLongTriggered = true;
    }
  } else if (!downNow && downWasPressed) {
    if (!downLongTriggered && (millis() - downPressStart < 3000)) {
      toolsSelected = (toolsSelected + 1) % TOOLS_COUNT;
      redrawSelection = true;
    }
    downWasPressed = false;
    downLongTriggered = false;
  }

  // OK button
  if (readButtonOnce(BTN_OK)) {
    if (toolsSelected == TOOLS_COUNT - 1) return true;   // exit
    bool newState = !toolStates[toolsSelected];
    setTool(toolsSelected, newState);
    int yPos = 55 + toolsSelected * 36;
    animateToggle(toolsSelected, newState, yPos);
    updateSingleToolUI(toolsSelected);
    redrawSelection = false;
  }

  if (redrawSelection) {
    // Redraw old and new selection (same as before)
    int oldY = 55 + old * 36;
    tft.fillRect(50, oldY - 8, 200, 20, BG);
    tft.setTextColor(GREEN, BG);
    tft.setCursor(50, oldY);
    tft.print(tools[old]);
    if (old < 3) {
      tft.setCursor(180, oldY);
      tft.setTextColor(toolStates[old] ? GREEN : RED, BG);
      tft.print(toolStates[old] ? "ON " : "OFF");
    }
    int newY = 55 + toolsSelected * 36;
    tft.fillRect(50, newY - 8, 200, 20, BG);
    tft.setTextColor(SELECT, BG);
    tft.setCursor(50, newY);
    tft.print(tools[toolsSelected]);
    if (toolsSelected < 3) {
      tft.setCursor(180, newY);
      tft.setTextColor(toolStates[toolsSelected] ? GREEN : RED, BG);
      tft.print(toolStates[toolsSelected] ? "ON " : "OFF");
    }
  }

  return false;
}

#endif