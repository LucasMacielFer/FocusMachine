#pragma once

#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include "pins.h"
#include "types.h"

#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSansBold24pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>

// UI colors
#define FOCUS_RED 0xF30A
#define LONG_BREAK_GREEN 0x5711
#define SHORT_BREAK_BLUE 0x651D

namespace Display
{
    // Functions called by the display task to interact with the TFT display
    void vClearDisplay(Adafruit_ILI9341 *tft);
    void vInitializeTimerDisplay(Adafruit_ILI9341 *tft, Types::PomodoroState state);
    void vPrintTemperature(Adafruit_ILI9341 *tft, float temperature);
    void vPrintHumidity(Adafruit_ILI9341 *tft, float humidity);
    void vPrintLuminosity(Adafruit_ILI9341 *tft, int luminosity);
    void vPrintFocusIndex(Adafruit_ILI9341 *tft, float focusIndex);
    void vPrintFocusIndexUponFocusEnd(Adafruit_ILI9341 *tft, float focusIndex);
    void vPrintComfortIndex(Adafruit_ILI9341 *tft, float comfortIndex);
    void vPrintTime(Adafruit_ILI9341 *tft, int pos, int value);
    void vPrintStateFinished(Adafruit_ILI9341 *tft, Types::PomodoroState state);
    void vPrintFaceDetectedOnAdjustment(Adafruit_ILI9341 *tft, bool detected);
    void vPrintAdjustingTimer(Adafruit_ILI9341 *tft, Types::PomodoroState state, int minutes, int seconds);
    void vPrintTimerAdjustment(Adafruit_ILI9341 *tft, int pos, int value, bool highlight);
} // namespace Display