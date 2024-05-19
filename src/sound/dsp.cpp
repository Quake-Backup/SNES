#include "dsp.h"

#include <cstdio>
#include <cstdlib>

void DSP::Write(uint8_t reg, uint8_t data)
{
    switch (reg)
    {
    default:
        printf("[DSP]: Unhandled write to reg %x (%x)\n", reg, data);
    }
}