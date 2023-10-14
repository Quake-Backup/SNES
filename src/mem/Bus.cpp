#include "Bus.h"
#include "../ppu/ppu.h"
#include "hdma.h"
#include "../cpu/cpu.h"

#include <fstream>

uint8_t* rom;
size_t rom_size;
uint8_t* ram;

void Bus::Init()
{
    std::ifstream file("doom.smc", std::ios::ate | std::ios::binary);
    rom_size = file.tellg();

    rom = new uint8_t[rom_size];

    file.seekg(0, std::ios::beg);
    file.read((char*)rom, rom_size);

    ram = new uint8_t[128*1024];
}

void Bus::Dump()
{
    std::ofstream dump("ram.bin");

    dump.write((char*)ram, 128*1024);
    dump.close();
}

uint8_t timeup;
uint8_t hvbjoy = 0;
uint8_t nmitimen = 0;
bool nmi_flag = false;

extern CPU* cpu;

void Bus::SetVblank(bool set)
{   
    if (set)
    {
        hvbjoy |= 0x80;

        if (nmitimen & 0x80)
            cpu->DoNMI();
        nmi_flag = true;
    }
    else
    {
        hvbjoy &= ~0x80;
        nmi_flag = false;
    }
}

uint8_t Bus::Read8(uint32_t addr)
{
    uint8_t bank = (addr >> 16) & 0xff;
    addr &= 0xFFFF;

    switch (bank)
    {
    case 0x00 ... 0x3F:
    {
        if (addr >= 0x8000)
            return rom[(((bank&0x3F) << 15) | (addr & 0x7fff)) & (rom_size-1)];
        if (addr < 0x2000)
            return ram[addr];

        switch (addr)
        {
        case 0x4211:
        {
            uint8_t data = timeup;
            timeup = 0;
            return data;
        }
        case 0x4210:
            nmi_flag = true;
            return nmi_flag;
        case 0x4212:
            return hvbjoy;
        case 0x4016:
            return 0;
        }
        
        printf("Read8 from unknown addr 0x%02x:0x%04x\n", bank, addr);
        exit(1);
    }
    case 0x40 ... 0x5F:
    {
        return rom[(((bank&0x3F) * 0x10000) + addr) & (rom_size-1)];

        printf("Read8 from unknown addr 0x%02x:0x%04x\n", bank, addr);
        exit(1);
    }
    case 0x7F:
        return ram[0x10000 + (addr & 0xFFFF)];
    default:
        printf("Read8 from unknown bank 0x%02x\n", bank);
        exit(1);
    }
}

uint16_t Bus::Read16(uint32_t addr)
{
    uint8_t bank = (addr >> 16) & 0xff;
    addr &= 0xFFFF;

    switch (bank)
    {
    case 0x00 ... 0x3F:
    {
        if (addr >= 0x8000)
            return *(uint16_t*)&rom[(((bank&0x3F) << 15) | (addr & 0x7fff)) & (rom_size-1)];
        if (addr < 0x2000)
            return *(uint16_t*)&ram[addr];
        
        switch (addr)
        {
        case 0x4211:
        {
            uint16_t data = (timeup << 16) | hvbjoy;
            timeup = 0;
            return data;
        }
        case 0x4212:
            return hvbjoy;
        case 0x4218 ... 0x421F:
            return 0;
        }
        
        printf("Read16 from unknown addr 0x%02x:0x%04x\n", bank, addr);
        exit(1);
    }
    case 0x40 ... 0x5F:
    {
        return *(uint16_t*)&rom[(((bank&0x3F) * 0x10000) + addr) & (rom_size-1)];

        printf("Read8 from unknown addr 0x%02x:0x%04x\n", bank, addr);
        exit(1);
    }
    case 0x7F:
        return *(uint16_t*)&ram[0x10000 + (addr & 0xFFFF)];
    default:
        printf("Read16 from unknown bank 0x%02x\n", bank);
        exit(1);
    }
}

uint8_t wrmpya, wrmpyb;
uint16_t multiply_result;

void Bus::Write8(uint32_t addr, uint8_t data)
{
    uint8_t bank = (addr >> 16) & 0xff;
    addr &= 0xFFFF;

    if (addr == 0x108 || addr == 0x109 || addr == 0x10A || addr == 0x10B)
    {
        printf("Writing 0x%02x to 0x%04x\n", data, addr);
    }

    switch (bank)
    {
    case 0x00 ... 0x3F:
    {
        if (addr < 0x2000)
        {
            ram[addr] = data;
            return;
        }

        switch (addr)
        {
        case 0x2100:
            PPU::WriteINIDISP(data);
            return;
        case 0x2105:
            PPU::WriteBGMODE(data);
            return;
        case 0x2107:
        case 0x2108:
        case 0x2109:
        case 0x210A:
            PPU::WriteBGTMAPSTART(addr - 0x2107, data);
            return;
        case 0x210D:
        case 0x210E:
            return;
        case 0x2115:
            PPU::WriteVMAIN(data);
            return;
        case 0x2118:
            PPU::WriteVMDATALow(data);
            return;
        case 0x2119:
            PPU::WriteVMDATAHi(data);
            return;
        case 0x2122:
            PPU::WriteCGDATA(data);
            return;
        case 0x2132:
            PPU::WriteCOLDATA(data);
            return;
        case 0x212C:
        case 0x212D:
        case 0x2130:
        case 0x2131:
        case 0x2133:
            return;
        case 0x4200:
            printf("[BUS]: Writing 0x%02x to NMITIMEN\n", data);
            nmitimen = data;
            return;
        case 0x4300:
            HDMA::WriteDMAPx(0, data);
            return;
        case 0x4301:
            HDMA::WriteBBADx(0, data);
            return;
        case 0x4304:
            HDMA::WriteTblBank(0, data);
            return;
        case 0x4305:
        case 0x4315:
        case 0x4325:
        case 0x4335:
        case 0x4345:
        case 0x4355:
        case 0x4365:
        case 0x4375:
            HDMA::WriteDASxL((addr >> 4) & 0xf, data);
            return;
        case 0x420B:
            HDMA::WriteMDMAEN(data);
            return;
        }

        if (((addr & 0xFF00) == 0x4200) || ((addr & 0xFF00) == 0x2100))
        {
            if (data == 0)
                return;
            else
            {
                printf("Uh oh, actual data sent to 0x%04x\n", addr);
                exit(1);
            }
        }

        printf("Write8 to unknown addr 0x%02x:0x%04x\n", bank, addr);
        exit(1);
    }
    case 0x7f:
    {
        ram[0x10000 + (addr&0xffff)] = data;
        return;
    }
    default:
        printf("Write8 to unknown bank 0x%02x\n", bank);
        exit(1);
    }
}

void Bus::Write16(uint32_t addr, uint16_t data)
{
    uint8_t bank = (addr >> 16) & 0xff;
    addr &= 0xFFFF;

    switch (bank)
    {
    case 0x00 ... 0x3F:
    {
        if (addr < 0x2000)
        {
            *(uint16_t*)&ram[addr] = data;
            return;
        }

        switch (addr)
        {
        case 0x2116:
            PPU::WriteVMADD(data);
            return;
        case 0x2118:
            PPU::WriteVMDATA(data);
            return;
        case 0x212C:
            return;
        case 0x4305:
        {
            HDMA::WriteDASxL(0, data);
            HDMA::WriteDASxH(0, data >> 8);
            return;
        }
        case 0x4302:
            HDMA::WriteTblStart(0, data);
            return;
        }

        printf("Write16 to unknown addr 0x%02x:0x%04x\n", bank, addr);
        exit(1);
    }
    case 0x7f:
    {
        *(uint16_t*)&ram[0x10000 + (addr&0xffff)] = data;
        return;
    }
    default:
        printf("Write16 to unknown bank 0x%02x\n", bank);
        exit(1);
    }
}