/*  
 *  Copyright 2018-2021 ledmaker.org
 *  
 *  This file is part of Glow Decompiler Lib.
 *  
 */

#include "public_api.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct Led Leds[LED_COUNT];
struct LedstripBuffer ledstripBuffer = { .leds = Leds, .numLeds = LED_COUNT, .isDirty = false };

void SetLedstripTestColor(uint8_t red, uint8_t green, uint8_t blue, uint8_t bright)
{
    // Set default color data:
    for (uint16_t ledIdx = 0; ledIdx < LED_COUNT; ledIdx++)
    {
        ledstripBuffer.leds[ledIdx].red = red;
        ledstripBuffer.leds[ledIdx].green = green;
        ledstripBuffer.leds[ledIdx].blue = blue;
        ledstripBuffer.leds[ledIdx].bright = bright;
    }

    // Push color data to ledstrip:
    ledstripBuffer.isDirty = true;
    ProgramLedstrip(&ledstripBuffer);
}