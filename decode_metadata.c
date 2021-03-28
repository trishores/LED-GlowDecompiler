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
#include "bit_handler.h"
#include "decode_instruction.h"
#include "decode_metadata.h"
#include "ledstrip_buffer.h"

#define BITS_PER_BYTE 8
#define BIT_BYTE_SHIFT 3

volatile struct GlobalContext gContext;
volatile struct PathContext pContext;

bool InitAnimation(bool isSaveToRom)
{
	gContext.ptrSram = ptrSramBufferStart;  // set pointer to start of allocated sram region

#ifdef NVM_BUF_START_ADDR
	if (isSaveToRom)
	{
		// Load known portion of metadata-region common data into sram:
		gContext.ptrNvm = NVM_BUF_START_ADDR;  // set pointer to start of allocated sram region.
		FlashRead(gContext.ptrNvm, gContext.ptrSram, 14);    // read first 14 bytes.
	}
#endif

	// Initialize metadata-region bit handler to start of sram-region:
	InitContextBitHandler(ptrSramBufferStart);

	// Read metadata-region common data:
	if (GetNextContextBitfieldValue(4) != Pc2Dev_ContextRegion)
	{
		return false; // abort if invalid/missing metadata-region.
	}

	gContext.contextRegionByteLen_Value = GetNextContextBitfieldValue(16);
	gContext.instrRegionByteLen_Value = GetNextContextBitfieldValue(32);
	gContext.totalLeds_Value = GetNextContextBitfieldValue(16);
	gContext.tickIntervalMs_Value = GetNextContextBitfieldValue(16);
	gContext.simBrightCoeff_Value = GetNextContextBitfieldValue(16);
	gContext.totalPaths_Value = GetNextContextBitfieldValue(8);
	pContext.isEndedBitfield_BitAddress = GetCurrentContextBitAddress();
	FastForwardContextBits(gContext.totalPaths_Value); // move bit handler past path-end bitmap to first metadata-block.
	gContext.firstContextBlock_BitAddress = GetCurrentContextBitAddress();  // save bit address of first metadata-block.
	gContext.nextContextBlock_BitAddress = gContext.firstContextBlock_BitAddress;
	pContext.pathIdx_Value = 0;  // initialize path idx.

    // Reset path-ended bitfield in common metadata:
    //pContext.isEnded_Value = 0;
    //for (int i = 1; i < gContext.totalPaths_Value; i++) pContext.isEnded_Value |= (1 << (gContext.totalPaths_Value - 1 - i));
    //SetContextBitfieldValue(pContext.isEndedBitfield_BitAddress, pContext.pathIdx_Value, pContext.isEnded_Value);
    // Reset extra value bitfields in each path's metadata: to do
    // Reset pause ticks bitfields in each path's metadata: to do

    // Update tick interval in timer:
    SetTickInterval(gContext.tickIntervalMs_Value);

	// Update brightness coefficient:
	SaveBrightnessCoefficient(gContext.simBrightCoeff_Value);

#ifdef NVM_BUF_START_ADDR
	if (isSaveToRom)
	{
		// Load entire metadata region into sram now that its length is known:
		FlashRead(NVM_BUF_START_ADDR, gContext.ptrSram, gContext.contextRegionByteLen_Value);
	}
#endif

	// Other initialization:
	InitInstrBitHandler(ptrSramBufferStart + gContext.contextRegionByteLen_Value); // initialize sram bit handler to start of first instr path.
	SetLedstripTestColor(0, 0, 0, 0);   // turn off all leds.

    return true;
}

bool RunAnimation(bool isSaveToRom)
{
	do
	{
		// Process next metadata-block data...

		// Get current path's is-ended value:
		pContext.isEnded_Value = GetContextBitfieldValue(pContext.isEndedBitfield_BitAddress + pContext.pathIdx_Value, 1);

		// Switch bit handler to start of metadata-region:
		SetCurrentContextBitAddress(gContext.nextContextBlock_BitAddress);
		// Get current path's start-byte address:
		uint8_t tickOpcode = GetNextContextBitfieldValue(2);
		pContext.pathStartByteAddress_Value = GetNextContextBitfieldValue((tickOpcode + 1) * BITS_PER_BYTE);

		if (!isSaveToRom)
		{
			// Reinit instruction bit handler to start of each instruction path (since instr bit addr is relative to start of current instr path):
			InitInstrBitHandler(ptrSramBufferStart + gContext.contextRegionByteLen_Value + pContext.pathStartByteAddress_Value);
		}

		// Get current path's byte length:
		tickOpcode = GetNextContextBitfieldValue(2);
		pContext.pathByteLen_Value = GetNextContextBitfieldValue((tickOpcode + 1) * BITS_PER_BYTE);
		// Get current path's instruction bit address:
		tickOpcode = GetNextContextBitfieldValue(2);
		pContext.instrBitAddressBitfield_BitAddress = GetCurrentContextBitAddress();
		pContext.instrBitAddressBitfield_BitWidth = (tickOpcode + 1) * BITS_PER_BYTE;
		pContext.instrBitAddress_Value = GetNextContextBitfieldValue(pContext.instrBitAddressBitfield_BitWidth);
		// Get current path's extra value:
		tickOpcode = GetNextContextBitfieldValue(3);
		pContext.extraValueBitfield_BitAddress = GetCurrentContextBitAddress();
		pContext.extraValueBitfield_BitWidth = tickOpcode * BITS_PER_BYTE;
		pContext.extraValue_Value = GetNextContextBitfieldValue(pContext.extraValueBitfield_BitWidth);    // returns zero if bit width is zero.
		// Get current path's pause-ticks value:
		tickOpcode = GetNextContextBitfieldValue(3);
		pContext.pauseTicksBitfield_BitAddress = GetCurrentContextBitAddress();
		pContext.pauseTicksBitfield_BitWidth = tickOpcode * BITS_PER_BYTE;
		pContext.pauseTicks_Value = GetNextContextBitfieldValue(pContext.pauseTicksBitfield_BitWidth);    // returns zero if bit width is zero.

		// Save current bit address in metadata region as start of next metadata-block:
		gContext.nextContextBlock_BitAddress = GetCurrentContextBitAddress();

		// Skip current instr path if ended or paused:
		if (pContext.isEnded_Value) continue;

		// Decrement pause-ticks if greater than zero
		// and skip current path if still non-zero:
		if (pContext.pauseTicks_Value)
		{
			pContext.pauseTicks_Value--;
			SetContextBitfieldValue(pContext.pauseTicksBitfield_BitAddress, pContext.pauseTicksBitfield_BitWidth, pContext.pauseTicks_Value);
			if (pContext.pauseTicks_Value) continue;
		}

		// Process current path's instructions...
#ifdef NVM_BUF_START_ADDR
		if (isSaveToRom)
		{
			// Context switch i.e. load current path's instructions into sram (immediately following metadata-region):
			gContext.ptrNvm = NVM_BUF_START_ADDR + gContext.contextRegionByteLen_Value + pContext.pathStartByteAddress_Value;  // flash start byte of current path.
			gContext.ptrSram = ptrSramBufferStart + gContext.contextRegionByteLen_Value;
			FlashRead(gContext.ptrNvm, gContext.ptrSram, pContext.pathByteLen_Value);
		}
#endif

		// Switch bit handler to start of instruction region:
		SetCurrentInstrBitAddress(pContext.instrBitAddress_Value);  // set bit handler to first bit of sram-loaded path instructions (byte->bit shifted).

		// Repeatedly process current path's instructions until path is complete or paused:
		while (ProcessNextInstruction()) { continue; };

		//printf("completed path=%d\n", pContext.pathIdx_Value);  // sim debugging.

	} while (++pContext.pathIdx_Value < gContext.totalPaths_Value);

	// Reset to first metadata region block:
	pContext.pathIdx_Value = 0;
	gContext.nextContextBlock_BitAddress = gContext.firstContextBlock_BitAddress;

	// Update ledstrip if ledstrip buffer is dirty:
	if (ledstripBuffer.isDirty) ProgramLedstrip(&ledstripBuffer);

	//printf("updated ledstrip...\n");  // sim debugging.

    return true;
}