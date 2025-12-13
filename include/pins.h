#pragma once

#include <Arduino.h>

#define TFT_SCLK    42
#define TFT_MOSI    41
#define TFT_MISO    40  
#define TFT_CS      38
#define TFT_DC      39
#define TFT_RST     -1  // ligar ao 3V3

// FUNÇÕES
#define PIN_PWM     21 // Buzzer
#define PIN_IN_DIG  47 // DHT22
#define PIN_IN_PIR  45 // PIR
#define PIN_IN_ANLG 14 // LDR
#define BUTTON_PIN  0  // Button

// TOUCH
#define PIN_TOUCH1  T1
#define PIN_TOUCH2  T2

namespace Pins
{
    // Function to initialize all pins
    void initializePins();
} // namespace Pins