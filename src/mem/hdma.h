#pragma once

#include <cstdint>
#include <cstdio>

namespace HDMA
{

void WriteDASxL(int chan, uint8_t data);
void WriteDASxH(int chan, uint8_t data);

void WriteTblStart(int chan, uint16_t data);
void WriteTblBank(int chan, uint8_t data);

void WriteDMAPx(int chan, uint8_t data);
void WriteBBADx(int chan, uint8_t data);

void WriteMDMAEN(uint8_t data);

}