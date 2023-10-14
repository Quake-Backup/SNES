#include "mem/Bus.h"
#include "cpu/cpu.h"
#include "ppu/ppu.h"

#include <cstdlib>

CPU* cpu;

void e()
{
    cpu->Dump();
    Bus::Dump();
    PPU::Dump();
}

int main()
{
    Bus::Init();
    PPU::Init();

    cpu = new CPU();

    std::atexit(e);

    PPU::Tick(34);

    while (1)
    {
        int cycles = cpu->Clock();
        cycles *= 8;
        PPU::Tick(cycles/4);
    }

    return 0;
}