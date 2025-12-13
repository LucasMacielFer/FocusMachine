#include "display.h"

namespace Display
{
    // Clears the entire display
    void vClearDisplay(Adafruit_ILI9341 *tft)
    {
        tft->fillScreen(ILI9341_BLACK);
    }

    // Initializes the timer display based on the current Pomodoro state
    void vInitializeTimerDisplay(Adafruit_ILI9341 *tft, Types::PomodoroState state)
    {
        int color;
        char buffer[20];
        int cursorX;
        vClearDisplay(tft);

        switch (state)
        {
        case Types::PomodoroState::FOCUS:
            color = FOCUS_RED;
            cursorX = 98;
            snprintf(buffer, sizeof(buffer), "FOCUS");
            break;
        case Types::PomodoroState::SHORT_BREAK:
            color = SHORT_BREAK_BLUE;
            cursorX = 32;
            snprintf(buffer, sizeof(buffer), "SHORT BREAK");
            break;
        case Types::PomodoroState::LONG_BREAK:
            color = LONG_BREAK_GREEN;
            cursorX = 43;
            snprintf(buffer, sizeof(buffer), "LONG BREAK");
            break;
        default:
            break;
        }

        // Draws the outer rectangle for the timer
        tft->drawRect(97, 63, 127, 49, color);

        // Prints the state name
        tft->setTextColor(color);
        tft->setFont(&FreeSansBold18pt7b);
        tft->setCursor(cursorX, 50);
        tft->print(buffer);

        // Metrics section
        tft->setTextColor(0xFFFF);
        tft->setFont(&FreeSans9pt7b);
        tft->setCursor(20, 141);
        tft->print("Temperature: ");
        tft->setCursor(20, 164);
        tft->print("Luminosity: ");
        tft->setCursor(20, 187);
        tft->print("Humidity: ");

        tft->setFont(&FreeSansBold9pt7b);
        tft->setCursor(20, 221);
        tft->print("Comfort:");
        tft->setCursor(177, 221);
        tft->print("Focus:");

        tft->setFont(&FreeSans9pt7b);
        tft->setCursor(240, 141);
        tft->print("--.- *C");
        tft->setCursor(239, 164);
        tft->print("--.-%");
        tft->setCursor(239, 187);
        tft->print("--.-%");
        tft->setCursor(99, 221);
        tft->print("--.-%");
        tft->setCursor(239, 221);
        tft->print("--.-%");
        tft->drawRect(13, 121, 294, 73, 0xFFFF);
        tft->drawRect(13, 204, 294, 24, 0xFFFF);
        tft->setTextSize(1);
        tft->setFont(&FreeSansBold24pt7b);
        tft->setCursor(154, 99);
        tft->print(":");
    }

    // Prints the temperature on the display
    void vPrintTemperature(Adafruit_ILI9341 *tft, float temperature)
    {
        char buffer[20];
        snprintf(buffer, sizeof(buffer), "%.1f C", temperature);

        tft->fillRect(240, 126, 60, 18, 0x0000);

        tft->setFont(&FreeSans9pt7b);
        tft->setCursor(240, 141);
        tft->print(buffer);
    }

    // Prints the humidity on the display
    void vPrintHumidity(Adafruit_ILI9341 *tft, float humidity)
    {
        char buffer[20];
        snprintf(buffer, sizeof(buffer), "%.1f%%", humidity);

        tft->fillRect(239, 172, 60, 18, 0x0000);

        tft->setFont(&FreeSans9pt7b);
        tft->setCursor(239, 187);
        tft->print(buffer);
    }

    // Prints the luminosity on the display
    void vPrintLuminosity(Adafruit_ILI9341 *tft, int luminosity)
    {
        char buffer[20];
        snprintf(buffer, sizeof(buffer), "%d%%", luminosity);

        tft->fillRect(239, 149, 60, 18, 0x0000);

        tft->setFont(&FreeSans9pt7b);
        tft->setCursor(239, 164);
        tft->print(buffer);
    }

    // Prints the focus index on the display
    void vPrintFocusIndex(Adafruit_ILI9341 *tft, float focusIndex)
    {
        char buffer[20];
        snprintf(buffer, sizeof(buffer), "%.1f%%", focusIndex);

        tft->fillRect(239, 206, 65, 18, 0x0000);

        tft->setFont(&FreeSansBold9pt7b);
        tft->setCursor(239, 221);
        tft->print(buffer);
    }

    // Prints the focus index prominently when focus session ends
    void vPrintFocusIndexUponFocusEnd(Adafruit_ILI9341 *tft, float focusIndex)
    {
        char buffer[20];
        snprintf(buffer, sizeof(buffer), "%.1f%%", focusIndex);
        tft->setTextColor(0xFFFF);
        tft->setFont(&FreeSansBold12pt7b);
        tft->setCursor(86, 148);
        tft->print("Focus score:");
        tft->setFont(&FreeSansBold24pt7b);
        tft->setCursor(94, 195);
        tft->print(buffer);
    }

    // Prints the comfort index on the display
    void vPrintComfortIndex(Adafruit_ILI9341 *tft, float comfortIndex)
    {
        char buffer[20];
        snprintf(buffer, sizeof(buffer), "%.1f%%", comfortIndex);

        tft->fillRect(99, 206, 60, 18, 0x0000);

        tft->setFont(&FreeSansBold9pt7b);
        tft->setCursor(99, 221);
        tft->print(buffer);
    }

    /* Prints the time on the display at the specified position
    * "pos" indicates which digit to update 
    * (0 = units of seconds, 1 = tens of seconds, 2 = units of minutes, 3 = tens of minutes)
    */
    void vPrintTime(Adafruit_ILI9341 *tft, int pos, int value)
    {
        char buffer[20];
        int posX = 0;
        snprintf(buffer, sizeof(buffer), "%d", value);

        switch (pos)
        {
        case 3: posX = 102; break;
        case 2: posX = 128; break;
        case 1: posX = 166; break;
        case 0: posX = 192; break;
        default: break;
        }

        tft->fillRect(posX, 71, 27, 35, 0x0000);

        tft->setFont(&FreeSansBold24pt7b);
        tft->setTextColor(0xFFFF);
        tft->setTextSize(1);
        tft->setCursor(posX, 104);
        tft->print(buffer);
    }

    // Prints the "FINISH" message when a Pomodoro state ends
    void vPrintStateFinished(Adafruit_ILI9341 *tft, Types::PomodoroState state)
    {
        vClearDisplay(tft);
        int color;

        switch (state)
        {
        case Types::PomodoroState::FOCUS:
            color = FOCUS_RED;
            break;
        case Types::PomodoroState::SHORT_BREAK:
            color = SHORT_BREAK_BLUE;
            break;
        case Types::PomodoroState::LONG_BREAK:
            color = LONG_BREAK_GREEN;
            break;
        default:
            break;
        }


        tft->fillRect(0, 0, 319, 100, color);
        tft->setTextColor(0x0);
        tft->setTextWrap(false);
        tft->setFont(&FreeSansBold24pt7b);
        tft->setCursor(79, 72);
        tft->print("FINISH");
    }

    /* Prints whether a face is detected during timer adjustment
    *  The purpose of this is to help the user to know if the
    *  camera is properly positioned.
    */
    void vPrintFaceDetectedOnAdjustment(Adafruit_ILI9341 *tft, bool detected)
    {
        tft->setFont(&FreeSansBold12pt7b);
        if(detected)
            tft->setTextColor(0xFFFF);
        else
            tft->setTextColor(0x0000);
            
        tft->setCursor(59, 220);
        tft->print("FACE DETECTED");
    }

    // Prints the timer adjustment screen
    void vPrintAdjustingTimer(Adafruit_ILI9341 *tft, Types::PomodoroState state, int minutes, int seconds)
    {
        int color;
        char buffer[20];
        char numBuffer[20];
        int cursorX;
        vClearDisplay(tft);

        switch (state)
        {
        case Types::PomodoroState::FOCUS:
            color = FOCUS_RED;
            cursorX = 118;
            snprintf(buffer, sizeof(buffer), "FOCUS");
            break;
        case Types::PomodoroState::SHORT_BREAK:
            color = SHORT_BREAK_BLUE;
            cursorX = 72;
            snprintf(buffer, sizeof(buffer), "SHORT BREAK");
            break;
        case Types::PomodoroState::LONG_BREAK:
            color = LONG_BREAK_GREEN;
            cursorX = 80;
            snprintf(buffer, sizeof(buffer), "LONG BREAK");
            break;
        default:
            break;
        }

        tft->setTextColor(0xFFFF);
        tft->setTextSize(2);
        tft->setTextWrap(false);
        tft->setFont(&FreeSansBold24pt7b);
        tft->setCursor(148, 166);
        tft->print(":");

        tft->setFont(&FreeSansBold24pt7b);
        tft->setTextColor(0xFFFF);
        tft->setTextSize(2);
        tft->setCursor(44, 175);
        snprintf(numBuffer, sizeof(numBuffer), "%d", minutes / 10);
        tft->print(numBuffer);

        tft->setTextColor(0x73AF);
        tft->setCursor(96, 175);
        snprintf(numBuffer, sizeof(numBuffer), "%d", minutes % 10);
        tft->print(numBuffer);

        tft->setCursor(172, 175);
        snprintf(numBuffer, sizeof(numBuffer), "%d", seconds / 10);
        tft->print(numBuffer);

        tft->setCursor(224, 175);
        snprintf(numBuffer, sizeof(numBuffer), "%d", seconds % 10);
        tft->print(numBuffer);   

        tft->setTextColor(color);
        tft->setTextSize(1);
        tft->setCursor(65, 45);
        tft->print("ADJUST");

        tft->fillRect(57, 59, 207, 35, color);

        tft->setTextColor(0x0);
        tft->setTextSize(1);
        tft->setFont(&FreeSansBold12pt7b);
        tft->setCursor(cursorX, 83);
        tft->print(buffer);
    }

    // Prints a specific digit of the timer adjustment screen
    void vPrintTimerAdjustment(Adafruit_ILI9341 *tft, int pos, int value, bool highlight)
    {
        char buffer[20];
        int posX = 0;
        int color = highlight ? 0xFFFF : 0x73AF;
        snprintf(buffer, sizeof(buffer), "%d", value);

        switch (pos)
        {
        case 3: posX = 44; break;
        case 2: posX = 96; break;
        case 1: posX = 172; break;
        case 0: posX = 224; break;
        default: break;
        }

        tft->fillRect(posX, 105, 52, 90, 0x0000);

        tft->setFont(&FreeSansBold24pt7b);
        tft->setTextColor(color);
        tft->setTextSize(2);
        tft->setCursor(posX, 175);
        tft->print(buffer);
        tft->setTextSize(1);
    }
} // namespace Display