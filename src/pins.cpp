#include "pins.h"

namespace Pins
{
    void initializePins()
    {
        pinMode(PIN_PWM, OUTPUT);
        ledcAttachPin(PIN_PWM, 0);
        ledcSetup(0, 500, 8); // Channel 0, 500 hZ, 8-bit resolution

        pinMode(PIN_IN_DIG, INPUT);
        pinMode(PIN_IN_PIR, INPUT);
        pinMode(PIN_IN_ANLG, INPUT);
    }
} // namespace Pins