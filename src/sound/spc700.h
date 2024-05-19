#pragma once

#include <cstdint>
#include <string>

namespace SPC700
{

void LoadIPL(std::string name);

void Reset();

void Dump();
void Tick(int cycles);

uint8_t ReadPort(uint8_t port);
void WritePort(uint8_t port, uint8_t data);

}