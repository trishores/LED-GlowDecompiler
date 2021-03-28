/*
 *  Copyright 2018-2021 ledmaker.org
 *
 *  This file is part of Glow Decompiler Lib.
 *
 *  Glow Decompiler Lib is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published
 *  by the Free Software Foundation, either version 3 of the License,
 *  or any later version.
 *
 *  Glow Decompiler Lib is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Glow Decompiler Lib. If not, see https://www.gnu.org/licenses/.
 **/

#ifndef PUBLIC_API_H_
#define PUBLIC_API_H_


#include <stdbool.h>
#include <stdint.h>

/**
 * Mandatory defines (declare externally to this library):
 *
 * GLOW_PROTOCOL_VERSION declares the currently supported Glow protocol version.
 * Corresponds to code file parameter: "device:protocolVersion".
 *
 * LED_COUNT declares the number of LEDs in the driven ledstrip.
 * Corresponds to glowscript parameter: "device:ledCount".
 *
 * SRAM_BUF_SZ declares the byte size of the SRAM region used for animation storage/cache.
 * Corresponds to glowscript parameter: "device:ramSpaceBytes".
 *
 * NVM_BUF_END_ADDR declares the start byte address of the ROM (flash) region used for animation storage.
 * NVM_BUF_START_ADDR declares the end byte address of the ROM (flash) region used for animation storage.
 * (NVM_BUF_END_ADDR - NVM_BUF_START_ADDR) corresponds to glowscript parameter: "device:romSpaceBytes".
 *
 **/


/**
 * Glow Decompiler Lib function that initializes a animation by loading it's metadata into cache and performing a
 * quick validity check. Prior to calling this function, Animation binary data must be present in SRAM or ROM.
 *
 * param[in]: isSaveToRom: Specifies whether animation binary data is in ROM region or SRAM region.
 *
 * return: Initialization status. Ensure that initialization succeeds before running a animation.
 **/
extern bool InitAnimation(bool isSaveToRom);

/**
 * Glow Decompiler Lib function that executes one-tick of an animation. Should be called repeatedly by a single
 * thread. For fast hardware that executes a single animation tick faster than the tick interval call
 * this function just once every tick interval to maintain animation timing. For slow hardware that
 * executes a single animation-tick slower than the tick interval call this function back to back,
 * however animation timing will be slower than the coded animation timing.
 *
 * param[in]: isSaveToRom: Must be same value as passed in to initialize animation.
 *
 * return: Run status. Returns false on animation completion. In that event, the animation can be
 *        restarted without calling the initialization function again.
 **/
extern bool RunAnimation(bool isSaveToRom);

/**
 * Glow Decompiler Lib test-function that pushes a single test color to all ledstrip leds.
 **/
void SetLedstripTestColor(uint8_t red, uint8_t green, uint8_t blue, uint8_t bright);

/**
 * Pointer declaration to SRAM_BUF_SZ bytes of SRAM memory.
 * You must define/allocate u8SramBuffer externally to Glow Decompiler Lib.
 *
 * If isSaveToRom is set to false: u8SramBuffer holds the animation binary data .
 * If isSaveToRom is set to true: u8SramBuffer only caches the animation binary data during animation.
 **/
extern uint8_t *ptrSramBufferStart;

/**
 * Glow Decompiler Lib definition of object containing LED color data.
 **/
struct Led
{
    uint8_t red;		// 0-255 range.
    uint8_t green;		// 0-255 range.
    uint8_t blue;		// 0-255 range.
    uint8_t bright;		// 0-31 range.
};

/**
 * Glow Decompiler Lib definition of object containing ledstrip color data and information.
 **/
struct LedstripBuffer
{
    struct Led *leds;       // pointer to ledstrip color data buffer.
    uint16_t numLeds;		// initialized value is LED_COUNT.
    bool isDirty;	 		// whether buffered ledstrip color data has changed since last write to ledstrip.
};

/**
 * Declaration for your function that pushes color data to your ledstrip (simulated or otherwise).
 * Implement this function externally to the Glow Decompiler Lib.
 *
 * param[in] ledstripBuffer: Pointer to buffer storing ledstrip color data.
 *
 * return: None
 **/
extern void ProgramLedstrip(struct LedstripBuffer *ledstripBuffer);

/**
 * Declaration for your function that changes the animation tick interval.
 * Implement this function externally to the Glow Decompiler Lib.
 *
 * param[in] tickIntervalMs: Millisecond tick interval.
 *
 * return: None
 **/
extern void SetTickInterval(uint16_t tickIntervalMs);

/**
 * Declaration for your function that receives the compiler-generated brightnessCoefficient value,
 * which is an approximation of the animation's peak light intensity. The brightnessCoefficient
 * value is the animation's maximum instantaneous: (redVal + greenVal + blueVal) * brightVal.
 * It can be used to dynamically scale animation brightness according to your requirements.
 * Eg.1: simulators may scale-up to maximize the screen-brightness range of an animation.
 * Eg.2: hardware may scale-down to hard-limit overly bright animations (for eye-safety or heat).
 * Your use of this value is entirely optional.
 * Implement this function externally to the Glow Decompiler Lib.
 *
 * param[in] brightnessCoeff: compiler-generated brightnessCoefficient value.
 *
 * return: None
 **/
extern void SaveBrightnessCoefficient(uint16_t brightnessCoeff);

/**
 * Declaration for your function that reads flash memory.
 * Implement this function externally to the Glow Decompiler Lib.
 *
 * param[in] srcAddr: start address of flash to read.
 * param[in] ptrBuffer: pointer to destination buffer.
 * param[in] length: number of flash bytes to read.
 *
 * return: None
 **/
extern void FlashRead(uint32_t srcAddr, uint8_t *ptrBuffer, uint32_t length);

/**
 * Declaration for your function that implements asserts.
 * Implement this function externally to the Glow Decompiler Lib.
 *
 * param[in] condition: test condition.
 *
 * return: None
 **/
extern void Assert(bool condition);

#endif /* PUBLIC_API_H_ **/