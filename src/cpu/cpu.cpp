#include "cpu.h"

#include "../mem/Bus.h"
#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <sstream>
#include <iomanip>

// #define printf(x, ...) 0

std::string hextonum(uint16_t hex, int pad)
{
    std::stringstream ss;
    ss << std::hex << std::setfill('0') << std::setw(pad) << hex;
    std::string s = ss.str();
    return s;
}

uint8_t CPU::ReadImm8()
{
    uint8_t data = Bus::Read8(pbr << 16 | pc);
    pc++;
    return data;
}

uint16_t CPU::ReadImm16()
{
    uint16_t data = Bus::Read16(pbr << 16 | pc);
    pc += 2;
    return data;
}

void CPU::SetAbs8(std::string &disasm, uint8_t data)
{
    uint16_t abs = Bus::Read16(pbr << 16 | pc);
    pc += 2;
    Bus::Write8(dbr << 16 | abs, data);
    disasm = "$" + hextonum(abs, 4);
}

void CPU::SetAbs16(std::string &disasm, uint16_t data)
{
    uint16_t abs = Bus::Read16(pbr << 16 | pc);
    pc += 2;
    Bus::Write16(dbr << 16 | abs, data);
    disasm = "$" + hextonum(abs, 4);
}

void CPU::SetFlag(Flags flag, bool set)
{
    if (set)
        p |= flag;
    else
        p &= ~flag;
}

bool CPU::GetFlag(Flags flag)
{
    return p & flag;
}


void CPU::Push8(uint8_t data)
{
    Bus::Write8(sp, data);
    sp--;
}

void CPU::Push16(uint16_t data)
{
    Push8(data >> 8);
    Push8(data & 0xff);
}

uint8_t CPU::Pop8()
{
    sp++;
    return Bus::Read8(sp);
}

uint16_t CPU::Pop16()
{
    uint8_t l = Pop8();
    uint8_t h = Pop8();

    return (h << 8) | l;
}

CPU::CPU()
{
    pc = Bus::Read16(0xFFFC);
    p = 0;
    dbr = 0;
    sp = 0x1FF;
    pbr = 0;
    a.full = x.full = y.full = 0;

    SetFlag(MF, 1);
    SetFlag(XBF, 1);

    opcodes[0x18] = std::bind(&CPU::ClcImp, this);
    opcodes[0x1B] = std::bind(&CPU::TcsImp, this);
    opcodes[0x5C] = std::bind(&CPU::JmpLng, this);
    opcodes[0x78] = std::bind(&CPU::SeiImp, this);
    opcodes[0x8D] = std::bind(&CPU::StaAbs, this);
    opcodes[0xA9] = std::bind(&CPU::LdaImm, this);
    opcodes[0xC2] = std::bind(&CPU::RepImm, this);
    opcodes[0xD8] = std::bind(&CPU::CldImp, this);
    opcodes[0xE2] = std::bind(&CPU::SepImm, this);
    opcodes[0xFB] = std::bind(&CPU::XceImp, this);

    printf("Reset vector is 0x%08x\n", pc);
}

void CPU::DoNMI()
{
    Push8(pbr);
    Push16(pc);
    Push8(p);
    SetFlag(IF, true);
    SetFlag(DF, false);
    pbr = 0;
    pc = Bus::Read16(0xFFEA);
    printf("[CPU]: DOING NMI\n");
}

int CPU::Clock()
{
    printf("0x%06x ", pbr << 16 | pc);

    uint8_t opcode = Bus::Read8(pbr << 16 | pc++);

    static char buf[4096];

    snprintf(buf, 4096, "\t\t\tA:%04x X:%04x Y:%04x S:%04x D:%04x DB:%02x %s%s%s%s%s%s%s%s ", a.full, x.full, y.full, sp, d, dbr,
            GetFlag(NF) ? "N" : "n",
            GetFlag(VF) ? "V" : "v",
            GetFlag(MF) ? "M" : "m",
            GetFlag(XBF) ? "X" : "x",
            GetFlag(DF) ? "D" : "d",
            GetFlag(IF) ? "I" : "i",
            GetFlag(ZF) ? "Z" : "z",
            GetFlag(CF) ? "C" : "c");

    if (!opcodes[opcode])
    {
        printf("Unknown opcode 0x%02x\n", opcode);
        exit(1);
    }

    int cycles = opcodes[opcode]();

    printf("%s\n", buf);
    
    return cycles;
}

void CPU::Dump()
{
    printf("pc\t->\t0x%06x\n", pbr << 16 | pc);
    printf("sp\t->\t0x%04x\n", sp);
    printf("a\t->\t0x%04x\n", a.full);
    printf("x\t->\t0x%04x\n", x.full);
    printf("y\t->\t0x%04x\n", y.full);
    printf("dbr\t->\t0x%02x\n", dbr);
    printf("pbr\t->\t0x%02x\n", pbr);
    printf("d\t->\t0x%04x\n", d);
    printf("[%s%s%s%s%s]\n", 
        e ? "e" : ".", 
        GetFlag(MF) ? "m" : ".", 
        GetFlag(XBF) ? "x" : ".", 
        GetFlag(NF) ? "n" : ".", 
        GetFlag(ZF) ? "z" : ".");
}

int CPU::ClcImp()
{
    SetFlag(CF, false);

    printf("clc     ");

    return 2;
}

int CPU::TcsImp()
{
    sp = a.full;
    printf("tcs     ");
    return 2;
}

int CPU::SeiImp()
{
    SetFlag(IF, true);

    printf("sei     ");

    return 2;
}

int CPU::LdaImm()
{
    if (GetFlag(MF))
    {
        a.lo = ReadImm8();
        SetFlag(NF, (a.lo >> 7) & 1);
        SetFlag(ZF, !a.lo);
        printf("lda #$%02x ", a.full);
        return 2;
    }
    else
    {
        a.full = ReadImm16();
        SetFlag(NF, (a.full >> 15) & 1);
        SetFlag(ZF, !a.full);
        printf("lda #$%04x ", a.full);
        return 3;
    }
}

int CPU::RepImm()
{
    uint8_t imm = ReadImm8();
    for (int i = 0; i < 8; i++)
    {
        if (imm & (1 << i))
            SetFlag((Flags)(1 << i), false);
    }
    printf("rep #$%x ", imm);
    return 3; 
}

int CPU::CldImp()
{
    SetFlag(DF, false);
    printf("cld     ");
    return 2;
}

int CPU::SepImm()
{
    uint8_t imm = ReadImm8();
    for (int i = 0; i < 8; i++)
    {
        if (imm & (1 << i))
            SetFlag((Flags)(1 << i), true);
    }
    printf("sep #$%x ", imm);
    return 3;
}

int CPU::XceImp()
{
    bool temp = GetFlag(CF);
    SetFlag(CF, e);
    e = temp;

    printf("xce     ");
    assert(!e);
    return 2;
}

int CPU::StaAbs()
{
    std::string disasm;
    int cycles;
    if (!GetFlag(MF))
    {
        SetAbs16(disasm, a.full);
        cycles = 5;
    }
    else
    {
        SetAbs8(disasm, a.lo);
        cycles = 4;
    }
    printf("sta %s ", disasm.c_str());
    return cycles;
}

int CPU::JmpLng()
{
    uint16_t new_pc = ReadImm16();
    pbr = ReadImm8();
    pc = new_pc;

    printf("jmp $%06x ", pbr << 16 | pc);
    return 4;
}
