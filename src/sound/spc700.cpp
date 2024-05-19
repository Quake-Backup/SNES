#include "spc700.h"

#include <fstream>
#include <cassert>
#include <stdio.h>
#include "dsp.h"

namespace SPC700
{

uint8_t a, x, y;
uint16_t pc;
uint8_t sp;
uint8_t psw;

uint8_t port0, port1, port2, port3;
uint8_t in_port0, in_port1, in_port2, in_port3; // Values sent from the SNES

uint8_t ram[64*1024];

uint8_t selected_dsp_reg = 0;

struct Timers
{
    uint8_t target, counter_internal, counter;
    bool started = false;
} timers[3];

void LoadIPL(std::string name)
{
    std::ifstream file(name, std::ios::ate | std::ios::binary);
    size_t size = file.tellg();

    assert(size == 0x40);

    file.seekg(0, std::ios::beg);
    file.read((char*)ram+0xFFC0, size);
}

void Reset()
{
    y = a = x = 0;
    sp = 0xFF;
    psw = 0;
    pc = 0xFFC0;
}

void Dump()
{
    printf("[SPC700]: A: %02x X: %02x Y: %02x SP: %02x P: %02x\n", a, x, y, sp, psw);
    printf("[SPC700]: PC: %04x\n", pc);
    printf("[SPC700]: Port0: %02x Port1: %02x Port2: %02x Port3: %02x\n", port0, port1, port2, port3);
    printf("[SPC700]: InPort0: %02x InPort1: %02x InPort2: %02x InPort3: %02x\n", in_port0, in_port1, in_port2, in_port3);
    printf("[SPC700]: Timers: %02x %02x %02x\n", timers[0].target, timers[1].target, timers[2].target);
    
    std::ofstream dump("spc700_ram.bin");

    dump.write((char*)ram, 64*1024);
    dump.close();
}

uint8_t Read8(uint16_t addr)
{
    if (addr >= 0xFC00)
        return ram[addr];
    if (addr < 0xF0)
        return ram[addr];
    if (addr >= 0x0100 && addr < 0xFFC0)
        return ram[addr];

    switch (addr)
    {
    case 0xF4:
        return in_port0;
    case 0xF5:
        return in_port1;
    case 0xF6:
        return in_port2;
    case 0xF7:
        return in_port3;
    case 0xFA ... 0xFC:
        return timers[addr-0xFA].target;
    case 0xFD ... 0xFF:
        return timers[addr-0xFD].counter;
    }
    
    printf("[SPC700]: Unhandled read from %04x\n", addr);
    exit(1);
}

void Write8(uint16_t addr, uint8_t data)
{
    if (addr < 0xF0)
    {
        ram[addr] = data;
        return;
    }
    if (addr >= 0x0100 && addr < 0xFFC0)
    {
        ram[addr] = data;
        return;
    }

    switch (addr)
    {
    case 0xF1:
        timers[0].started = (data & 1);
        timers[1].started = (data & 2);
        timers[2].started = (data & 4);
        if ((data >> 4) & 1)
            port0 = port1 = in_port0 = in_port1 = 0;
        if ((data >> 5) & 1)
            port2 = port3 = in_port2 = in_port3 = 0;
        return;
    case 0xF3:
        DSP::Write(selected_dsp_reg, data);
        return;
    case 0xF4:
        port0 = data;
        return;
    case 0xF5:
        port1 = data;
        return;
    case 0xF6:
        port2 = data;
        return;
    case 0xF7:
        port3 = data;
        return;
    case 0xFA ... 0xFC:
        timers[addr-0xFA].target = data;
        return;
    }

    printf("[SPC700]: Unhandled write to %04x\n", addr);
    exit(1);
}

enum Flags
{
    Carry = 1 << 0,
    Zero = 1 << 1,
    IRQDisable = 1 << 2,
    HalfCarry = 1 << 3,
    Break = 1 << 4,
    Always1 = 1 << 5,
    Overflow = 1 << 6,
    Negative = 1 << 7,
};

void SetFlag(Flags flag, bool set)
{
    if (set)
        psw |= flag;
    else
        psw &= ~flag;
}

bool GetFlag(Flags flag)
{
    return psw & flag;
}

// #define printf(x, ...)

int OrADpX() // 0x07
{
    uint16_t ptr_addr = Read8(pc++);
    uint16_t addr = Read8(ptr_addr);

    printf("[SPC700]: or a, ($%02x+x)", addr);

    a |= Read8(addr+x);
    SetFlag(Flags::Zero, a == 0);
    SetFlag(Flags::Negative, a & 0x80);
    return 6;
}

int BplRel() // 0x10
{
    int8_t rel = Read8(pc++);
    printf("[SPC700]: bpl %04x", pc + rel);
    if (!GetFlag(Flags::Negative))
    {
        pc += rel;
        return 4;
    }
    return 2;
}

int DecX() // 0x1D
{
    printf("[SPC700]: dec x");
    x--;
    SetFlag(Flags::Zero, x == 0);
    SetFlag(Flags::Negative, x & 0x80);
    return 2;
}

int JmpAbx() // 0x1F
{
    uint16_t ptr_addr = Read8(pc++);
    ptr_addr |= Read8(pc++) << 8;
    ptr_addr += x;
    uint16_t addr = Read8(ptr_addr);
    addr |= Read8(ptr_addr+1) << 8;

    printf("[SPC700]: jmp [#$%04x+x]", ptr_addr);
    pc = addr;
    return 6;
}

int AndAImm() // 0x28
{
    uint8_t imm = Read8(pc++);
    printf("[SPC700]: and a, #%02x", imm);
    a &= imm;
    SetFlag(Flags::Zero, a == 0);
    SetFlag(Flags::Negative, a & 0x80);
    return 2;
}

int CbneDpRel() // 0x2E
{
    int cycles = 5;
    uint16_t addr = Read8(pc++);
    int8_t rel = Read8(pc++);
    printf("[SPC700]: cbne $%02x, %04x", addr, pc + rel);
    uint8_t data = Read8(addr);
    if (a != data)
    {
        pc += rel;
        cycles += 2;
    }
    return cycles;
}

int BraRel() // 0x2F
{
    int8_t rel = Read8(pc++);
    printf("[SPC700]: bra %04x", pc + rel);
    pc += rel;
    return 2;
}

int Bbc1Rel() // 0x33
{
    uint16_t addr = Read8(pc++);
    int8_t rel = Read8(pc++);
    printf("[SPC700]: bbc $%02x.1, %04x", addr, pc + rel);
    uint8_t data = Read8(addr);
    if (!(data & (1 << 1)))
    {
        pc += rel;
        return 7;
    }
    return 5;
}

int IncX() // 0x3D
{
    printf("[SPC700]: inc x");
    x++;
    SetFlag(Flags::Zero, x == 0);
    SetFlag(Flags::Negative, x & 0x80);
    return 2;
}

int CmpXDp() // 0x3E
{
    uint16_t addr = Read8(pc++);
    printf("[SPC700]: cmp x, $%02x", addr);
    uint8_t data = Read8(addr);
    SetFlag(Flags::Carry, data >= x);
    SetFlag(Flags::Zero, data == x);
    SetFlag(Flags::Negative, (x - data) & 0x80);
    return 3;
}

int CallAbs() // 0x3F
{
    uint16_t addr = Read8(pc++);
    addr |= Read8(pc++) << 8;
    printf("[SPC700]: call $%04x", addr);
    Write8(0x100 | sp--, pc >> 8);
    Write8(0x100 | sp--, pc & 0xFF);
    pc = addr;
    return 8;
}

int EorAImm() // 0x48
{
    uint8_t imm = Read8(pc++);
    printf("[SPC700]: eor a, #%02x", imm);
    a ^= imm;
    SetFlag(Flags::Zero, a == 0);
    SetFlag(Flags::Negative, a & 0x80);
    return 2;
}

int EorDptrY()
{
    uint16_t ptr_addr = Read8(pc++);
    uint16_t addr = Read8(ptr_addr) + y;

    printf("[SPC700]: eor a, [$%02x]+y", ptr_addr);

    a ^= Read8(addr);
    SetFlag(Flags::Zero, a == 0);
    SetFlag(Flags::Negative, a & 0x80);
    return 6;
}

int MovXA()
{
    printf("[SPC700]: mov x, a");
    x = a;
    SetFlag(Flags::Zero, x == 0);
    SetFlag(Flags::Negative, x & 0x80);
    return 2;
}

int JmpAbs()
{
    uint16_t addr = Read8(pc++);
    addr |= Read8(pc++) << 8;
    printf("[SPC700]: jmp $%04x", addr);
    pc = addr;
    return 3;
}

int Set3Dp() // 0x62
{
    uint16_t addr = Read8(pc++);
    printf("[SPC700]: set1 $%02x.3", addr);
    uint8_t data = Read8(addr);
    data |= 1 << 3;
    Write8(addr, data);
    return 5;
}

int Bbs3DpRel() // 0x63
{
    uint16_t addr = Read8(pc++);
    int8_t rel = Read8(pc++);
    printf("[SPC700]: bbs $%02x.3, %04x", addr, pc + rel);
    uint8_t data = Read8(addr);
    if (data & (1 << 3))
    {
        pc += rel;
        return 7;
    }
    return 5;
}

int CmpADp() // 0x64
{
    uint16_t addr = Read8(pc++);
    printf("[SPC700]: cmp a, $%02x", addr);
    uint8_t data = Read8(addr);
    SetFlag(Flags::Carry, a >= data);
    SetFlag(Flags::Zero, data == a);
    SetFlag(Flags::Negative, (a - data) & 0x80);
    return 5;
}

int CmpAImm() // 0x68
{
    uint8_t imm = Read8(pc++);
    printf("[SPC700]: cmp a, #%02x", imm);
    SetFlag(Flags::Carry, a >= imm);
    SetFlag(Flags::Zero, imm == a);
    SetFlag(Flags::Negative, (a - imm) & 0x80);
    return 2;
}

int CmpDpDp() // 0x69
{
    uint16_t addr1 = Read8(pc++);
    uint16_t addr2 = Read8(pc++);
    printf("[SPC700]: cmp $%02x, $%02x", addr2, addr1);
    uint8_t data1 = Read8(addr1);
    uint8_t data2 = Read8(addr2);
    SetFlag(Flags::Carry, data1 >= data2);
    SetFlag(Flags::Zero, data2 == data1);
    SetFlag(Flags::Negative, (data1 - data2) & 0x80);
    return 5;
}

int Ret() // 0x6F
{
    printf("[SPC700]: ret");
    pc = Read8(0x100 | ++sp);
    pc |= Read8(0x100 | ++sp) << 8;
    return 5;
}

int CmpDPImm() // 0x78
{
    uint8_t imm = Read8(pc++);
    uint16_t addr = Read8(pc++);
    printf("[SPC700]: cmp $%02x, #$%02x", addr, imm);
    uint8_t data = Read8(addr);
    SetFlag(Flags::Carry, data >= imm);
    SetFlag(Flags::Zero, data == imm);
    SetFlag(Flags::Negative, (data - imm) & 0x80);
    return 5;
}

int MovAX() // 0x7D
{
    a = x;
    printf("[SPC700]: mov a, x");
    SetFlag(Flags::Zero, a == 0);
    SetFlag(Flags::Negative, a & 0x80);
    return 2;
}

int CmpYDP() // 0x7E
{
    uint16_t addr = Read8(pc++);
    uint8_t data = Read8(addr);
    printf("[SPC700]: cmp y, #$%02x ($%02x, $%02x)", addr, data, y);
    SetFlag(Flags::Carry, y >= data);
    SetFlag(Flags::Zero, data == y);
    SetFlag(Flags::Negative, (y - data) & 0x80);
    return 5;
}

int MovDPImm() // 0x8F
{
    uint8_t imm = Read8(pc++);
    uint16_t addr = Read8(pc++);
    printf("[SPC700]: mov $%02x, #%02x", addr, imm);
    Write8(addr, imm);
    if (imm != 0xAA && addr == 0xF4 && (pc & 0xFF00) == 0x000) exit(1);
    return 5;
}

int BccRel() // 0x90
{
    int8_t rel = Read8(pc++);
    printf("[SPC700]: bcc %04x", pc + rel);
    if (!GetFlag(Flags::Carry))
        pc += rel;
    return 2;
}

int IncDp() // 0xAB
{
    uint16_t addr = Read8(pc++);
    printf("[SPC700]: inc $%02x", addr);
    uint8_t data = Read8(addr);
    data++;
    Write8(addr, data);
    SetFlag(Flags::Zero, data == 0);
    SetFlag(Flags::Negative, data & 0x80);
    return 5;
}

int MovXIncA()
{
    uint16_t addr = x;
    printf("[SPC700]: mov (x)+, a");
    Write8(addr, a);
    x++;
    return 4;
}

int BcsRel() // 0xB0
{
    int8_t rel = Read8(pc++);
    printf("[SPC700]: bcs %04x", pc + rel);
    if (GetFlag(Flags::Carry))
    {
        pc += rel;
        return 4;
    }
    return 2;
}

int MovwYADP() // 0xBA
{
    uint16_t addr = Read8(pc++);
    printf("[SPC700]: movw ya, #%02x", addr);
    a = Read8(addr);
    y = Read8(addr+1);
    SetFlag(Flags::Zero, a == 0 && y == 0);
    SetFlag(Flags::Negative, a & 0x80);
    return 5;
}

int MovSPX() // 0xBD
{
    printf("[SPC700]: mov sp, x");
    sp = x;
    return 2;
}

int MovDpA() // 0xC4
{
    uint16_t addr = Read8(pc++);
    printf("[SPC700]: mov $%02x, a", addr);
    Write8(addr, a);
    return 5;
}

// 0xC6
int MovDirXA()
{
    uint16_t addr = x;
    printf("[SPC700]: mov (x), a");
    Write8(addr, a);
    return 4;
}

int CmpXImm() // 0xC8
{
    uint8_t imm = Read8(pc++);
    printf("[SPC700]: cmp x, #%02x", imm);
    SetFlag(Flags::Carry, x >= imm);
    SetFlag(Flags::Zero, x == imm);
    SetFlag(Flags::Negative, (x - imm) & 0x80);
    return 2;
}

int MovDPY()
{
    uint16_t addr = Read8(pc++);
    printf("[SPC700]: mov $%02x, y", addr);
    Write8(addr, y);
    return 5;
}

int MovXImm() // 0xCD
{
    uint8_t imm = Read8(pc++);
    x = imm;
    printf("[SPC700]: mov x, #%02x", imm);
    SetFlag(Flags::Zero, imm == 0);
    SetFlag(Flags::Negative, imm & 0x80);
    return 2;
}

int BneRel() // 0xD0
{
    int8_t rel = Read8(pc++);
    printf("[SPC700]: bne %04x", pc + rel);
    if (!GetFlag(Flags::Zero))
        pc += rel;
    return 2;
}

int MovAbXA() // 0xD5
{
    uint16_t addr = Read8(pc++);
    addr |= Read8(pc++) << 8;
    addr += x;
    printf("[SPC700]: mov [#$%04x+x], a", addr-x);
    Write8(addr, a);
    return 6;
}

int MovDPtrYA() // 0xD7
{
    uint16_t ptr_addr = Read8(pc++);
    uint16_t addr = Read8(ptr_addr);
    addr |= Read8(ptr_addr+1) << 8;

    printf("[SPC700]: mov (dp)+y, a ($%04x)", addr);

    Write8(addr+y, a);
    return 7;
}

int MovDpX()
{
    uint16_t addr = Read8(pc++);
    printf("[SPC700]: mov $%02x, x", addr);
    Write8(addr, x);
    return 4;
}

int MovWDPYA() // 0xDA
{
    uint16_t addr = Read8(pc++);
    printf("[SPC700]: movw #%02x, ya ($%04x)", addr, y << 4 | a);
    Write8(addr, a);
    Write8(addr+1, y);
    return 5;
}

int MovAY() // 0xDD
{
    printf("[SPC700]: mov a, y");
    a = y;
    SetFlag(Flags::Zero, a == 0);
    SetFlag(Flags::Negative, a & 0x80);
    return 2;
}

int MovADp()
{
    uint16_t addr = Read8(pc++);
    printf("[SPC700]: mov a, $%02x", addr);
    a = Read8(addr);
    SetFlag(Flags::Zero, a == 0);
    SetFlag(Flags::Negative, a & 0x80);
    return 5;
}

int MovAImm() // 0xE8
{
    uint8_t imm = Read8(pc++);
    a = imm;
    printf("[SPC700]: mov a, #%02x", imm);
    SetFlag(Flags::Zero, imm == 0);
    SetFlag(Flags::Negative, imm & 0x80);
    return 2;
}

int MovYDP() // 0xEB
{
    uint16_t addr = Read8(pc++);
    printf("[SPC700]: mov y, $%02x", addr);
    y = Read8(addr);
    SetFlag(Flags::Zero, y == 0);
    SetFlag(Flags::Negative, y & 0x80);
    return 5;
}

int BeqRel() // 0xF0
{
    int8_t rel = Read8(pc++);
    printf("[SPC700]: beq %04x", pc + rel);
    if (GetFlag(Flags::Zero))
        pc += rel;
    return 2;
}

int MovAAbX()
{
    uint16_t addr = Read8(pc++);
    addr |= Read8(pc++) << 8;
    addr += x;
    printf("[SPC700]: mov a, [#$%04x+x]", addr);
    a = Read8(addr);
    SetFlag(Flags::Zero, a == 0);
    SetFlag(Flags::Negative, a & 0x80);
    return 5;
}

int MovDpDp() // 0xFA
{
    uint16_t addr1 = Read8(pc++);
    uint16_t addr2 = Read8(pc++);
    printf("[SPC700]: mov $%02x, $%02x", addr2, addr1);
    Write8(addr1, Read8(addr2));
    return 5;
}

int IncY() // 0xFC
{
    printf("[SPC700]: inc y");
    y++;
    SetFlag(Flags::Zero, y == 0);
    SetFlag(Flags::Negative, y & 0x80);
    return 2;
}

int MovYA()
{
    y = a;
    printf("[SPC700]: mov y, a");
    SetFlag(Flags::Zero, y == 0);
    SetFlag(Flags::Negative, y & 0x80);
    return 2;
}

#undef printf

void Tick(int cycles)
{
    for (int cycle = 0; cycle < cycles;)
    {
        uint8_t opcode = Read8(pc++);

        int elapsed_cycles;

        switch (opcode)
        {
        case 0x00:
            elapsed_cycles = 2;
            break;
        case 0x07:
            elapsed_cycles = OrADpX();
            break;
        case 0x10:
            elapsed_cycles = BplRel();
            break;
        case 0x1D:
            elapsed_cycles = DecX();
            break;
        case 0x1F:
            elapsed_cycles = JmpAbx();
            break;
        case 0x28:
            elapsed_cycles = AndAImm();
            break;
        case 0x2E:
            elapsed_cycles = CbneDpRel();
            break;
        case 0x2F:
            elapsed_cycles = BraRel();
            break;
        case 0x33:
            elapsed_cycles = Bbc1Rel();
            break;
        case 0x3D:
            elapsed_cycles = IncX();
            break;
        case 0x3E:
            elapsed_cycles = CmpXDp();
            break;
        case 0x3F:
            elapsed_cycles = CallAbs();
            break;
        case 0x48:
            elapsed_cycles = EorAImm();
            break;
        case 0x57:
            elapsed_cycles = EorDptrY();
            break;
        case 0x5D:
            elapsed_cycles = MovXA();
            break;
        case 0x5F:
            elapsed_cycles = JmpAbs();
            break;
        case 0x62:
            elapsed_cycles = Set3Dp();
            break;
        case 0x63:
            elapsed_cycles = Bbs3DpRel();
            exit(1);
        case 0x64:
            elapsed_cycles = CmpADp();
            break;
        case 0x68:
            elapsed_cycles = CmpAImm();
            break;
        case 0x69:
            elapsed_cycles = CmpDpDp();
            break;
        case 0x6F:
            elapsed_cycles = Ret();
            break;
        case 0x78:
            elapsed_cycles = CmpDPImm();
            break;
        case 0x7D:
            elapsed_cycles = MovAX();
            break;
        case 0x7E:
            elapsed_cycles = CmpYDP();
            break;
        case 0x8F:
            elapsed_cycles = MovDPImm();
            break;
        case 0x90:
            elapsed_cycles = BccRel();
            break;
        case 0xAB:
            elapsed_cycles = IncDp();
            break;
        case 0xAF:
            elapsed_cycles = MovXIncA();
            break;
        case 0xB0:
            elapsed_cycles = BcsRel();
            break;
        case 0xBA:
            elapsed_cycles = MovwYADP();
            break;
        case 0xBD:
            elapsed_cycles = MovSPX();
            break;
        case 0xC4:
            elapsed_cycles = MovDpA();
            break;
        case 0xC6:
            elapsed_cycles = MovDirXA();
            break;
        case 0xC8:
            elapsed_cycles = CmpXImm();
            break;
        case 0xCB:
            elapsed_cycles = MovDPY();
            break;
        case 0xCD:
            elapsed_cycles = MovXImm();
            break;
        case 0xD0:
            elapsed_cycles = BneRel();
            break;
        case 0xD5:
            elapsed_cycles = MovAbXA();
            break;
        case 0xD7:
            elapsed_cycles = MovDPtrYA();
            break;
        case 0xD8:
            elapsed_cycles = MovDpX();
            break;
        case 0xDA:
            elapsed_cycles = MovWDPYA();
            break;
        case 0xDD:
            elapsed_cycles = MovAY();
            break;
        case 0xE4:
            elapsed_cycles = MovADp();
            break;
        case 0xE8:
            elapsed_cycles = MovAImm();
            break;
        case 0xEB:
            elapsed_cycles = MovYDP();
            break;
        case 0xF0:
            elapsed_cycles = BeqRel();
            break;
        case 0xF5:
            elapsed_cycles = MovAAbX();
            break;
        case 0xFA:
            elapsed_cycles = MovDpDp();
            break;
        case 0xFC:
            elapsed_cycles = IncY();
            break;
        case 0xFD:
            elapsed_cycles = MovYA();
            break;
        default:
            printf("[SPC700]: Unknown opcode %02x\n", opcode);
            exit(1);
        }

        printf(" A: %02x X: %02x Y: %02x SP: %02x P: %02x\n", a, x, y, sp, psw);

        cycle += elapsed_cycles;
        for (int i = 0; i < 3; i++)
        {
            if (timers[i].started)
            {
                timers[i].counter_internal++;
                if (timers[i].counter_internal == timers[i].target)
                {
                    timers[i].counter_internal = 0;
                    timers[i].counter++;
                }
            }
        }
    }
}

uint8_t ReadPort(uint8_t port)
{
    switch (port)
    {
    case 0:
        return port0;
    case 1:
        return port1;
    case 2:
        return port2;
    case 3:
        return port3;
    default:
        printf("[SPC700]: Unknown port %d\n", port);
        exit(1);
    }
}

void WritePort(uint8_t port, uint8_t data)
{
    printf("[SPC700]: Write port %d = %02x\n", port, data);
    switch (port)
    {
    case 0:
        in_port0 = data;
        break;
    case 1:
        in_port1 = data;
        break;
    case 2:
        in_port2 = data;
        break;
    case 3:
        in_port3 = data;
        break;
    default:
        printf("[SPC700]: Unknown port %d\n", port);
        exit(1);
    }
}
}