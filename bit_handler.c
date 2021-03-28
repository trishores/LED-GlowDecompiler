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

#define BITS_PER_BYTE 8

static uint8_t *bufInstrStartPtr;
static uint8_t *bufContextStartPtr;
static volatile uint32_t bitInstrIndex;
static volatile uint32_t bitContextIndex;

void InitInstrBitHandler(uint8_t *startPtr)
{
	bufInstrStartPtr = startPtr;
	bitInstrIndex = 0;
}

void InitContextBitHandler(uint8_t *startPtr)
{
	bufContextStartPtr = startPtr;
	bitContextIndex = 0;
}

uint32_t GetCurrentInstrBitAddress()
{
    return bitInstrIndex;
}

uint32_t GetCurrentContextBitAddress()
{
    return bitContextIndex;
}

void SetCurrentInstrBitAddress(uint32_t bitAddress)
{
    bitInstrIndex = bitAddress;	// bit address is relative to start of buffer.
}

void SetCurrentContextBitAddress(uint32_t bitAddress)
{
    bitContextIndex = bitAddress;	// bit address is relative to start of buffer.
}

void FastForwardInstrBits(uint32_t numBits)
{
	bitInstrIndex += numBits;
}

void FastForwardContextBits(uint32_t numBits)
{
	bitContextIndex += numBits;
}

static void SetBitfieldValue(uint8_t *startPtr, uint32_t bitAddress, uint8_t bitfieldWidth, uint32_t u32Val)
{
    Assert(bitfieldWidth <= 32);

	uint32_t endBitIndex = bitAddress + bitfieldWidth - 1;
	uint32_t startByteIndex = bitAddress / 8;
	uint32_t endByteIndex = endBitIndex / 8;
	uint8_t j = 0;
	uint8_t leftShift = 7 - (endBitIndex % 8);
	uint64_t u64Mask = 1;
	u64Mask = ~(((u64Mask << bitfieldWidth) - 1) << leftShift);
	uint64_t u64Val = u32Val;
	u64Val = u64Val << leftShift;

	do
	{
		// Store buffer bytes containing bitfield:
		startPtr[endByteIndex] &= ((u64Mask >> j) & 0xFF);
		startPtr[endByteIndex] |= ((u64Val >> j) & 0xFF);
		j += 8;
		if (endByteIndex == 0) break;	// avoid endByteIndex being compared after underflow.
	} while (endByteIndex-- > startByteIndex);
}

//void SetInstrBitfieldValue(uint32_t bitAddress, uint8_t bitfieldWidth, uint32_t u32Val)
//{
//    SetBitfieldValue(bufInstrStartPtr, bitAddress, bitfieldWidth, u32Val);
//}

void SetContextBitfieldValue(uint32_t bitAddress, uint8_t bitfieldWidth, uint32_t u32Val)
{
    SetBitfieldValue(bufContextStartPtr, bitAddress, bitfieldWidth, u32Val);
}

static uint32_t GetBitfieldValue(uint8_t *startPtr, uint32_t bitAddress, uint8_t bitfieldWidth)
{
    Assert(bitfieldWidth <= 32);

	uint32_t endBitIndex = bitAddress + bitfieldWidth - 1;
	uint32_t leftByteIndex = bitAddress / 8;	// must be signed.
	uint32_t rightByteIndex = endBitIndex / 8;	// must be signed.
	uint8_t j = 0, rightShift, leftShift;
	uint64_t u64Temp, u64Val = 0;

	// Process each byte from rightByteIndex to leftByteIndex (inclusive):
	do
	{
		// Load bytes containing bitfield:
		u64Temp = startPtr[rightByteIndex];
		u64Val |= u64Temp << j;
		j += 8;
		if (rightByteIndex == 0) break;	// avoid rightByteIndex being compared after underflow.
	} while (rightByteIndex-- > leftByteIndex);

	// Remove non-bitfield bits:
	rightShift = 7 - (endBitIndex % 8);
	leftShift = 64 - (bitfieldWidth + rightShift);
	u64Temp = u64Val << leftShift;
	u64Val = u64Temp >> (leftShift + rightShift);

	return (uint32_t)u64Val;
}

uint32_t GetInstrBitfieldValue(uint32_t bitAddress, uint8_t bitfieldWidth)
{
    return GetBitfieldValue(bufInstrStartPtr, bitAddress, bitfieldWidth);
}

uint32_t GetContextBitfieldValue(uint32_t bitAddress, uint8_t bitfieldWidth)
{
    return GetBitfieldValue(bufContextStartPtr, bitAddress, bitfieldWidth);
}

uint32_t GetNextInstrBitfieldValue(uint8_t bitfieldWidth)
{
    if (bitfieldWidth == 0) return 0;   // handle case where opcode indicates absent value field.

    uint32_t u32Val = GetBitfieldValue(bufInstrStartPtr, bitInstrIndex, bitfieldWidth);

    bitInstrIndex += bitfieldWidth;

    return u32Val;
}

uint32_t GetNextContextBitfieldValue(uint8_t bitfieldWidth)
{
    if (bitfieldWidth == 0) return 0;   // handle case where opcode indicates absent value field.

    uint32_t u32Val = GetBitfieldValue(bufContextStartPtr, bitContextIndex, bitfieldWidth);

    bitContextIndex += bitfieldWidth;

    return u32Val;
}