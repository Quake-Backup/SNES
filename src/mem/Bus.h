#pragma once

#include <cstdint>

namespace Bus
{

void Init();
void Dump();

void SetVblank(bool set);

uint8_t Read8(uint32_t addr);
uint16_t Read16(uint32_t addr);

void Write8(uint32_t addr, uint8_t data);
void Write16(uint32_t addr, uint16_t data);

}