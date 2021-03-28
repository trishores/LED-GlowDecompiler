/*  
 *  Copyright 2018-2021 ledmaker.org
 *  
 *  This file is part of Glow Decompiler Lib.
 *  
 */

#ifndef DECODE_METADATA_H_
#define DECODE_METADATA_H_

enum Instr
{
	Pc2Dev_Here = 1,
	Pc2Dev_Goto = 2,
	Pc2Dev_Pause = 3,
	Pc2Dev_GlowImmediate = 4,
	Pc2Dev_GlowRamp = 5,
	Pc2Dev_ContextRegion = 13,
	Pc2Dev_PathActivate = 14,
	Pc2Dev_PathEnd = 15
};

struct GlobalContext
{
    uint32_t ptrNvm;
    uint8_t *ptrSram;
    enum Instr currInstr;
    uint8_t totalPaths_Value;
    uint32_t instrRegionByteLen_Value;
    uint16_t contextRegionByteLen_Value;
    uint16_t totalLeds_Value;
    uint16_t tickIntervalMs_Value;
    uint16_t simBrightCoeff_Value;
    uint16_t firstContextBlock_BitAddress;
    uint32_t nextContextBlock_BitAddress;
};
extern volatile struct GlobalContext gContext;

struct PathContext
{
    // Current path data:
    uint8_t pathIdx_Value;
    uint32_t pathStartByteAddress_Value;
    uint32_t pathByteLen_Value;
    uint32_t isEnded_Value;
    uint16_t isEndedBitfield_BitAddress;
    uint32_t instrBitAddress_Value;
    uint32_t instrBitAddressBitfield_BitAddress;
    uint8_t instrBitAddressBitfield_BitWidth;
    uint32_t pauseTicks_Value;
    uint32_t pauseTicksBitfield_BitAddress;
    uint8_t pauseTicksBitfield_BitWidth;
    uint32_t extraValue_Value;
    uint32_t extraValueBitfield_BitAddress;
    uint8_t extraValueBitfield_BitWidth;
};
extern volatile struct PathContext pContext;

#endif /* DECODE_METADATA_H_ */