#include "hdma.h"

#include <cstdlib>
#include <cstdio>
#include <cassert>

#include "Bus.h"

struct Channel
{
    uint16_t byteCount = 0;
    uint32_t startAddr = 0;
    uint8_t dmap = 0;
    uint8_t bbus;
} chans[8];

void HDMA::WriteDASxL(int chan, uint8_t data)
{
    printf("Writing %x to DASxL\n", data);
    chans[chan].byteCount &= ~0xFF;
    chans[chan].byteCount |= data;
}

void HDMA::WriteDASxH(int chan, uint8_t data)
{
    printf("Writing %x to DASxH\n", data);
    chans[chan].byteCount &= 0xFF;
    chans[chan].byteCount |= (data << 8);
}

void HDMA::WriteTblStart(int chan, uint16_t data)
{
    chans[chan].startAddr &= ~0xFFFF;
    chans[chan].startAddr |= data;
}

void HDMA::WriteTblBank(int chan, uint8_t data)
{
    chans[chan].startAddr &= 0xFFFF;
    chans[chan].startAddr |= ((uint32_t)data << 16);
}

void HDMA::WriteDMAPx(int chan, uint8_t data)
{
    chans[chan].dmap = data;
}

void HDMA::WriteBBADx(int chan, uint8_t data)
{
    chans[chan].bbus = data;
}

void HDMA::WriteMDMAEN(uint8_t data)
{
    if (!data)
        return;

    if (data != 0x1)
    {
        printf("TODO: Channels other than 1 being started (0x%02x)\n", data);
        exit(1);
    }

    auto& c = chans[0];
#define printf(x, ...) 0
    printf("[HDMA]: Transferring %d bytes (step %d reg %x type %d)\n", c.byteCount, (c.dmap >> 3) & 3, c.bbus, c.dmap & 0x7);

    uint8_t step = (c.dmap >> 3) & 3;

    assert((c.byteCount % 2) == 0);

    if ((c.dmap & 0x7) == 1)
    {
        while (c.byteCount)
        {
            printf("[HDMA]: Reading from 0x%04x\n", c.startAddr);
            uint16_t data = Bus::Read16(c.startAddr);
            if (step == 0)
                c.startAddr = (c.startAddr & 0xFF0000) | ((uint16_t)(c.startAddr & 0xFFFF) + 2);
            else if (step == 2)
                c.startAddr = (c.startAddr & 0xFF0000) | ((uint16_t)(c.startAddr & 0xFFFF) - 2);

            Bus::Write16(0x2100 + c.bbus, data);
            c.byteCount -= 2;
        }
    }
    else if ((c.dmap & 0x7) == 2)
    {
        while (c.byteCount)
        {
            uint16_t data = Bus::Read16(c.startAddr);
            if (step == 0)
                c.startAddr = (c.startAddr & 0xFF0000) | ((uint16_t)(c.startAddr & 0xFFFF) + 2);
            else if (step == 2)
                c.startAddr = (c.startAddr & 0xFF0000) | ((uint16_t)(c.startAddr & 0xFFFF) - 2);

            Bus::Write8(0x2100 + c.bbus, data >> 8);
            Bus::Write8(0x2100 + c.bbus, data);
            c.byteCount -= 2;
        }
    }
    else
    {
        assert(0);
    }
}
