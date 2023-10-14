#pragma once

#include <cstdint>

namespace PPU
{

void Init();
void Tick(int cycles);
void Dump();

void WriteINIDISP(uint8_t data);
void WriteCOLDATA(uint8_t data);
void WriteBGMODE(uint8_t data);
void WriteBGTMAPSTART(int index, uint8_t data);

void WriteVMADD(uint16_t data);
void WriteVMAIN(uint8_t data);

void WriteVMDATA(uint16_t data);
void WriteVMDATALow(uint8_t data);
void WriteVMDATAHi(uint8_t data);

void WriteCGDATA(uint8_t data);

}