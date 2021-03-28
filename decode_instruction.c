/*  
 *  Copyright 2018-2021 ledmaker.org
 *  
 *  This file is part of Glow Decompiler Lib.
 *  
 */

#include "public_api.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "decode_instruction.h"
#include "decode_metadata.h"
#include "bit_handler.h"
#include "ledstrip_buffer.h"

#define RED_MASK 0x08
#define GREEN_MASK 0x04
#define BLUE_MASK 0x02
#define BRIGHT_MASK 0x01

#define BITS_PER_BYTE 8
#define BIT_BYTE_SHIFT 3

typedef enum
{
    SetVal = 0,
    SetAllZeroThenVal = 1,
    SetVal0 = 2,
    SetValRandom = 3
} ActionOpcode;

bool ProcessPathActivate()
{
    // Set specified path as runnable, the path's pause-ticks and instr-bit-addr should already be reset:
    uint8_t pathIdx = (uint8_t)GetNextInstrBitfieldValue(8);
    SetContextBitfieldValue(pContext.isEndedBitfield_BitAddress + pathIdx, 1, false);

    return false;   // return unblocked.
}

bool ProcessGlowImmediate()
{
    uint8_t colorBitmap = GetNextInstrBitfieldValue(4);
    ActionOpcode actionOpcode = GetNextInstrBitfieldValue(2);

    bool isNewRed = false, isNewGreen = false, isNewBlue = false, isNewBright = false;

    if (actionOpcode == SetAllZeroThenVal)	// turn all leds off then set absolute color value(s).
    {
        SetLedstripTestColor(0, 0, 0, 0);
        actionOpcode = SetVal;
    }

    if (actionOpcode == SetVal)	// set absolute value(s).
    {
        uint8_t red = 0, green = 0, blue = 0, bright = 0;
        if (colorBitmap & RED_MASK)
        {
            isNewRed = true;
            red = GetNextInstrBitfieldValue(8);
        }
        if (colorBitmap & GREEN_MASK)
        {
            isNewGreen = true;
            green = GetNextInstrBitfieldValue(8);
        }
        if (colorBitmap & BLUE_MASK)
        {
            isNewBlue = true;
            blue = GetNextInstrBitfieldValue(8);
        }
        if (colorBitmap & BRIGHT_MASK)
        {
            isNewBright = true;
            bright = GetNextInstrBitfieldValue(5);
        }

        for (uint16_t ledIdx = 0; ledIdx < ledstripBuffer.numLeds; ledIdx++)
        {
            bool isLedActive = (bool)GetNextInstrBitfieldValue(1);
            if (!isLedActive) continue;

            if (isNewRed)
            {
                ledstripBuffer.leds[ledIdx].red = red;
            }
            if (isNewGreen)
            {
                ledstripBuffer.leds[ledIdx].green = green;
            }
            if (isNewBlue)
            {
                ledstripBuffer.leds[ledIdx].blue = blue;
            }
            if (isNewBright)
            {
                ledstripBuffer.leds[ledIdx].bright = bright;
            }
            if (isNewRed || isNewGreen || isNewBlue || isNewBright)
            {
				ledstripBuffer.isDirty = true;
            }
        }
    }

    return false;   // return unblocked.
}

bool ProcessGlowRamp()
{
    uint32_t glowRampStartBitAddress = GetCurrentInstrBitAddress() - 4;
    uint8_t tickOpcode = GetNextInstrBitfieldValue(2);
    uint32_t rampTicksVal = GetNextInstrBitfieldValue((tickOpcode + 1) * BITS_PER_BYTE);
    uint32_t rampTicksCounter = pContext.extraValue_Value;
    uint8_t colorBitmap = GetNextInstrBitfieldValue(4);

    // Increment ramp tick counter and save to metadata-block:
    if (rampTicksCounter > rampTicksVal) rampTicksCounter = 0;
    pContext.extraValue_Value = rampTicksCounter + 1;
    SetContextBitfieldValue(pContext.extraValueBitfield_BitAddress, pContext.extraValueBitfield_BitWidth, pContext.extraValue_Value);

	bool isNewRed = false, isNewGreen = false, isNewBlue = false, isNewBright = false;
	uint8_t incDecOp, redColorVal = 0, greenColorVal = 0, blueColorVal = 0, brightColorVal = 0, colorStep, colorOffset;
	uint32_t tickStep;

	if (colorBitmap & RED_MASK)
    {
		isNewRed = true;
		redColorVal = GetNextInstrBitfieldValue(8);  // start of ramp color.
		incDecOp = GetNextInstrBitfieldValue(2);
		if (incDecOp)
		{
        	tickOpcode = GetNextInstrBitfieldValue(2);
        	tickStep = GetNextInstrBitfieldValue((tickOpcode + 1) * BITS_PER_BYTE);
			colorStep = GetNextInstrBitfieldValue(8);
			colorOffset = (rampTicksCounter / tickStep) * colorStep;
			if (incDecOp == 1)	// increment.
			{
				if (colorOffset > 255 - redColorVal) redColorVal = 255;
				else redColorVal = redColorVal + colorOffset;
			}
			else if (incDecOp == 2)	// decrement.
			{
				if (colorOffset > redColorVal) redColorVal = 0;
				else redColorVal = redColorVal - colorOffset;
			}
    	}
	}

    if (colorBitmap & GREEN_MASK)
    {
		isNewGreen = true;
		greenColorVal = GetNextInstrBitfieldValue(8);  // start of ramp color.
		incDecOp = GetNextInstrBitfieldValue(2);
		if (incDecOp)
		{
        	tickOpcode = GetNextInstrBitfieldValue(2);
        	tickStep = GetNextInstrBitfieldValue((tickOpcode + 1) * BITS_PER_BYTE);
			colorStep = GetNextInstrBitfieldValue(8);
			colorOffset = (rampTicksCounter / tickStep) * colorStep;
			if (incDecOp == 1)	// increment.
			{
				if (colorOffset > 255 - greenColorVal) greenColorVal = 255;
				else greenColorVal = greenColorVal + colorOffset;
			}
			else if (incDecOp == 2)	// decrement.
			{
				if (colorOffset > greenColorVal) greenColorVal = 0;
				else greenColorVal = greenColorVal - colorOffset;
			}
		}
    }

    if (colorBitmap & BLUE_MASK)
    {
		isNewBlue = true;
		blueColorVal = GetNextInstrBitfieldValue(8);  // start of ramp color.
		incDecOp = GetNextInstrBitfieldValue(2);
		if (incDecOp)
		{
        	tickOpcode = GetNextInstrBitfieldValue(2);
        	tickStep = GetNextInstrBitfieldValue((tickOpcode + 1) * BITS_PER_BYTE);
			colorStep = GetNextInstrBitfieldValue(8);
			colorOffset = (rampTicksCounter / tickStep) * colorStep;
			if (incDecOp == 1)	// increment.
			{
				if (colorOffset > 255 - blueColorVal) blueColorVal = 255;
				else blueColorVal = blueColorVal + colorOffset;
			}
			else if (incDecOp == 2)	// decrement.
			{
				if (colorOffset > blueColorVal) blueColorVal = 0;
				else blueColorVal = blueColorVal - colorOffset;
			}
		}
    }

    if (colorBitmap & BRIGHT_MASK)
    {
		isNewBright = true;
		brightColorVal = GetNextInstrBitfieldValue(8);  // start of ramp color.
		incDecOp = GetNextInstrBitfieldValue(2);
		if (incDecOp)
		{
        	tickOpcode = GetNextInstrBitfieldValue(2);
        	tickStep = GetNextInstrBitfieldValue((tickOpcode + 1) * BITS_PER_BYTE);
			colorStep = GetNextInstrBitfieldValue(8);
			colorOffset = (rampTicksCounter / tickStep) * colorStep;
			if (incDecOp == 1)	// increment.
			{
				if (colorOffset > 255 - brightColorVal) brightColorVal = 255;
				else brightColorVal = brightColorVal + colorOffset;
			}
			else if (incDecOp == 2)	// decrement.
			{
				if (colorOffset > brightColorVal) brightColorVal = 0;
				else brightColorVal = brightColorVal - colorOffset;
			}
    	}
    }

    // Apply RGBW values to all affected leds:
    for (int ledIdx = 0; ledIdx < ledstripBuffer.numLeds; ledIdx++)
    {
        bool isLedActive = (bool)GetNextInstrBitfieldValue(1);
        if (!isLedActive) continue;

        if (isNewRed)
        {
            ledstripBuffer.leds[ledIdx].red = redColorVal;
        }

        if (isNewGreen)
        {
            ledstripBuffer.leds[ledIdx].green = greenColorVal;
        }

        if (isNewBlue)
        {
            ledstripBuffer.leds[ledIdx].blue = blueColorVal;
        }

        if (isNewBright)
        {
            ledstripBuffer.leds[ledIdx].bright = brightColorVal;
        }

		if (isNewRed || isNewGreen || isNewBlue || isNewBright)
		{
			ledstripBuffer.isDirty = true;
		}
    }

    if (rampTicksCounter++ < rampTicksVal)
    {
        // Set/save new pause value:
        pContext.pauseTicks_Value = 1;
        SetContextBitfieldValue(pContext.pauseTicksBitfield_BitAddress, pContext.pauseTicksBitfield_BitWidth, pContext.pauseTicks_Value);

        // Set current bit address to start of this ramp instruction in readiness for pause completion:
        pContext.instrBitAddress_Value = glowRampStartBitAddress;
        SetContextBitfieldValue(pContext.instrBitAddressBitfield_BitAddress, pContext.instrBitAddressBitfield_BitWidth, pContext.instrBitAddress_Value);
        return true;    // return blocked flag (paused).
    }

    return false;   // return unblocked flag (ramp instr completed).
}

bool ProcessPause()
{
    uint8_t tickOpcode = GetNextInstrBitfieldValue(2);
    pContext.pauseTicks_Value = GetNextInstrBitfieldValue((tickOpcode + 1) * BITS_PER_BYTE);
    SetContextBitfieldValue(pContext.pauseTicksBitfield_BitAddress, pContext.pauseTicksBitfield_BitWidth, pContext.pauseTicks_Value);

    // Save current bit address to start of path in readiness for pause completion:
    pContext.instrBitAddress_Value = GetCurrentInstrBitAddress();
    SetContextBitfieldValue(pContext.instrBitAddressBitfield_BitAddress, pContext.instrBitAddressBitfield_BitWidth, pContext.instrBitAddress_Value);

    return true;   // return blocked flag.
}

bool ProcessGoto()
{
    // Set current instr bit address to target instruction bit address:
    volatile uint32_t test = GetNextInstrBitfieldValue(32);
    SetCurrentInstrBitAddress(test);

    return false;   // return unblocked.
}

bool ProcessPathEnd()
{
    // Set is-ended bit:
    pContext.isEnded_Value = 1;
    SetContextBitfieldValue(pContext.isEndedBitfield_BitAddress + pContext.pathIdx_Value, 1, pContext.isEnded_Value);

    // Clear pause ticks in readiness for subsequent path activation:
    pContext.pauseTicks_Value = 0;
    SetContextBitfieldValue(pContext.pauseTicksBitfield_BitAddress, pContext.pauseTicksBitfield_BitWidth, pContext.pauseTicks_Value);

    // Set instruction bit address to start of path in readiness for next activation:
    pContext.instrBitAddress_Value = 0;
    SetContextBitfieldValue(pContext.instrBitAddressBitfield_BitAddress, pContext.instrBitAddressBitfield_BitWidth, pContext.instrBitAddress_Value);

    return true;   // return blocked flag.
}

bool ProcessNextInstruction()
{
    bool isBlocked = false;
    gContext.currInstr = GetNextInstrBitfieldValue(4);

    if (gContext.currInstr == Pc2Dev_PathActivate)
    {
        ProcessPathActivate();
        isBlocked = false;
    }
    else if (gContext.currInstr == Pc2Dev_GlowImmediate)
    {
        ProcessGlowImmediate();
        isBlocked = false;
    }
    else if (gContext.currInstr == Pc2Dev_GlowRamp)
    {
        isBlocked = ProcessGlowRamp();
    }
    else if (gContext.currInstr == Pc2Dev_Pause)
    {
        ProcessPause();
        isBlocked = true;
    }
    else if (gContext.currInstr == Pc2Dev_Here)
    {
        // skip over 'here' instruction.
        isBlocked = false;
    }
    else if (gContext.currInstr == Pc2Dev_Goto)
    {
        ProcessGoto();
        isBlocked = false;
    }
    else if (gContext.currInstr == Pc2Dev_PathEnd)
    {
        ProcessPathEnd();
        isBlocked = true;
    }

    return !isBlocked;
}
