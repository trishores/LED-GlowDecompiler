/*  
 *  Copyright 2018-2021 ledmaker.org
 *  
 *  This file is part of Glow Decompiler Lib.
 *  
 */

#ifndef BIT_HANDLER_H_
#define BIT_HANDLER_H_

/**
 * Initialize bit handler with buffer address/size and reset bit pointer.
**/
extern void InitInstrBitHandler(uint8_t *startPtr);
extern void InitContextBitHandler(uint8_t *startPtr);

/**
 * Get bit pointer address.
**/
extern uint32_t GetCurrentInstrBitAddress();
extern uint32_t GetCurrentContextBitAddress();

/**
 * Set bit pointer address.
**/
extern void SetCurrentInstrBitAddress(uint32_t bitAddress);
extern void SetCurrentContextBitAddress(uint32_t bitAddress);

/**
 * Jump bit pointer N bits.
**/
extern void FastForwardInstrBits(uint32_t numBits);
extern void FastForwardContextBits(uint32_t numBits);

/**
 * Get value of N bits starting at buffer bit address.
**/
extern uint32_t GetInstrBitfieldValue(uint32_t bitAddress, uint8_t bitfieldWidth);
extern uint32_t GetContextBitfieldValue(uint32_t bitAddress, uint8_t bitfieldWidth);

/**
 * Set value of N bits starting at buffer bit address.
**/
//extern void SetInstrBitfieldValue(uint32_t bitAddress, uint8_t bitfieldWidth, uint32_t value);
extern void SetContextBitfieldValue(uint32_t bitAddress, uint8_t bitfieldWidth, uint32_t value);

/**
 * Fetch value of next N bits in buffer. The requested N bits are returned in a right justified uint32_t.
**/
extern uint32_t GetNextInstrBitfieldValue(uint8_t bitfieldWidth);
extern uint32_t GetNextContextBitfieldValue(uint8_t bitfieldWidth);

#endif
