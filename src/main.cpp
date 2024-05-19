#include "mem/Bus.h"
#include "cpu/cpu.h"
#include "ppu/ppu.h"
#include "sound/spc700.h"

#include <cstdlib>
#include <SDL2/SDL.h>

CPU* cpu;

void e()
{
    cpu->Dump();
    Bus::Dump();
    PPU::Dump();
    SPC700::Dump();
}

uint64_t a, b;
double delta;

int main()
{
    Bus::Init();
    PPU::Init();
    SPC700::LoadIPL("spc700.rom");
    SPC700::Reset();

    cpu = new CPU();

    std::atexit(e);

    PPU::Tick(34);

    while (1)
    {
        int cycles = 0;
        for (int i = 0; i < 4; i++)
            cycles += cpu->Clock();
        PPU::Tick((cycles*8)/4);
        SPC700::Tick((int)(cycles / 1.3358209));
    }

    return 0;
}