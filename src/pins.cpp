#include "pins.h"

namespace Pins
{
    // Pin initializations
    void initializePins()
    {
        pinMode(PIN_PWM, OUTPUT);
        ledcAttachPin(PIN_PWM, 0);
        ledcSetup(0, 1000, 8); // Channel 0, 1000 Hz, 8-bit resolution
        ledcWrite(0, 0);

        pinMode(PIN_IN_DIG, INPUT);
        pinMode(PIN_IN_PIR, INPUT);
        pinMode(PIN_IN_ANLG, INPUT);
        pinMode(BUTTON_PIN, INPUT_PULLUP);
    }
} // namespace Pins