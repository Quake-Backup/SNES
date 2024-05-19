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

uint16_t CPU::GetDirXAddr()
{
    return d + ReadImm8() + x.lo;
}

uint16_t CPU::GetAbsXAddr()
{
    return dbr << 16 | (ReadImm16() + x.full);
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

    opcodes[0x04] = std::bind(&CPU::TsbDir, this);
    opcodes[0x08] = std::bind(&CPU::PhpImp, this);
    opcodes[0x09] = std::bind(&CPU::OraImm, this);
    opcodes[0x0A] = std::bind(&CPU::AslImp, this);
    opcodes[0x0B] = std::bind(&CPU::PhdImp, this);
    opcodes[0x10] = std::bind(&CPU::BplRel, this);
    opcodes[0x18] = std::bind(&CPU::ClcImp, this);
    opcodes[0x1A] = std::bind(&CPU::IncImp, this);
    opcodes[0x1B] = std::bind(&CPU::TcsImp, this);
    opcodes[0x20] = std::bind(&CPU::JsrAbs, this);
    opcodes[0x22] = std::bind(&CPU::JslLng, this);
    opcodes[0x26] = std::bind(&CPU::RolDir, this);
    opcodes[0x28] = std::bind(&CPU::PlpImp, this);
    opcodes[0x29] = std::bind(&CPU::AndImm, this);
    opcodes[0x2A] = std::bind(&CPU::RolImp, this);
    opcodes[0x2B] = std::bind(&CPU::PldImp, this);
    opcodes[0x2C] = std::bind(&CPU::BitAbs, this);
    opcodes[0x30] = std::bind(&CPU::BmiRel, this);
    opcodes[0x38] = std::bind(&CPU::SecImp, this);
    opcodes[0x3A] = std::bind(&CPU::DecAcc, this);
    opcodes[0x40] = std::bind(&CPU::RtiImp, this);
    opcodes[0x48] = std::bind(&CPU::PhaImp, this);
    opcodes[0x49] = std::bind(&CPU::EorImm, this);
    opcodes[0x4A] = std::bind(&CPU::LsrImp, this);
    opcodes[0x4B] = std::bind(&CPU::PhkImp, this);
    opcodes[0x4C] = std::bind(&CPU::JmpAbs, this);
    opcodes[0x5A] = std::bind(&CPU::PhyImp, this);
    opcodes[0x5B] = std::bind(&CPU::TcdImp, this);
    opcodes[0x5C] = std::bind(&CPU::JmpLng, this);
    opcodes[0x60] = std::bind(&CPU::RtsImp, this);
    opcodes[0x64] = std::bind(&CPU::StzDir, this);
    opcodes[0x65] = std::bind(&CPU::AdcDir, this);
    opcodes[0x67] = std::bind(&CPU::AdcDrP, this);
    opcodes[0x68] = std::bind(&CPU::PlaImp, this);
    opcodes[0x69] = std::bind(&CPU::AdcImm, this); // Nice
    opcodes[0x6A] = std::bind(&CPU::RorImp, this);
    opcodes[0x6B] = std::bind(&CPU::RtlImp, this);
    opcodes[0x6C] = std::bind(&CPU::JmpAbP, this);
    opcodes[0x74] = std::bind(&CPU::StzDrx, this);
    opcodes[0x78] = std::bind(&CPU::SeiImp, this);
    opcodes[0x7A] = std::bind(&CPU::PlyImp, this);
    opcodes[0x7B] = std::bind(&CPU::TdcImp, this);
    opcodes[0x80] = std::bind(&CPU::BraRel, this);
    opcodes[0x84] = std::bind(&CPU::StyDir, this);
    opcodes[0x85] = std::bind(&CPU::StaDir, this);
    opcodes[0x86] = std::bind(&CPU::StxDir, this);
    opcodes[0x88] = std::bind(&CPU::DeyImp, this);
    opcodes[0x89] = std::bind(&CPU::BitImm, this);
    opcodes[0x8A] = std::bind(&CPU::TxaImp, this);
    opcodes[0x8B] = std::bind(&CPU::PhbImp, this);
    opcodes[0x8C] = std::bind(&CPU::StyAbs, this);
    opcodes[0x8D] = std::bind(&CPU::StaAbs, this);
    opcodes[0x8E] = std::bind(&CPU::StxAbs, this);
    opcodes[0x90] = std::bind(&CPU::BccRel, this);
    opcodes[0x92] = std::bind(&CPU::StaDrp, this);
    opcodes[0x98] = std::bind(&CPU::TyaImp, this);
    opcodes[0x99] = std::bind(&CPU::StaAbY, this);
    opcodes[0x9B] = std::bind(&CPU::TxyImp, this);
    opcodes[0x9C] = std::bind(&CPU::StzAbs, this);
    opcodes[0x9D] = std::bind(&CPU::StaAbx, this);
    opcodes[0x9E] = std::bind(&CPU::StzAbx, this);
    opcodes[0xA0] = std::bind(&CPU::LdyImm, this);
    opcodes[0xA2] = std::bind(&CPU::LdxImm, this);
    opcodes[0xA4] = std::bind(&CPU::LdyDir, this);
    opcodes[0xA5] = std::bind(&CPU::LdaDir, this);
    opcodes[0xA7] = std::bind(&CPU::LdaDPL, this);
    opcodes[0xA8] = std::bind(&CPU::TayImp, this);
    opcodes[0xA9] = std::bind(&CPU::LdaImm, this);
    opcodes[0xAA] = std::bind(&CPU::TaxImp, this);
    opcodes[0xAB] = std::bind(&CPU::PlbImp, this);
    opcodes[0xAD] = std::bind(&CPU::LdaAbs, this);
    opcodes[0xAE] = std::bind(&CPU::LdxAbs, this);
    opcodes[0xB0] = std::bind(&CPU::BcsRel, this);
    opcodes[0xB2] = std::bind(&CPU::LdaDpt, this);
    opcodes[0xB7] = std::bind(&CPU::LdaDPY, this);
    opcodes[0xBB] = std::bind(&CPU::TyxImp, this);
    opcodes[0xBD] = std::bind(&CPU::LdaAbx, this);
    opcodes[0xBF] = std::bind(&CPU::LdaLnX, this);
    opcodes[0xC0] = std::bind(&CPU::CpyImm, this);
    opcodes[0xC2] = std::bind(&CPU::RepImm, this);
    opcodes[0xC4] = std::bind(&CPU::CpyDir, this);
    opcodes[0xC6] = std::bind(&CPU::DecDir, this);
    opcodes[0xC8] = std::bind(&CPU::InyImp, this);
    opcodes[0xC9] = std::bind(&CPU::CmpImm, this);
    opcodes[0xCA] = std::bind(&CPU::DexImp, this);
    opcodes[0xCD] = std::bind(&CPU::CmpAbs, this);
    opcodes[0xD0] = std::bind(&CPU::BneRel, this);
    opcodes[0xD8] = std::bind(&CPU::CldImp, this);
    opcodes[0xDA] = std::bind(&CPU::PhxImp, this);
    opcodes[0xE0] = std::bind(&CPU::CpxImm, this);
    opcodes[0xE2] = std::bind(&CPU::SepImm, this);
    opcodes[0xE5] = std::bind(&CPU::SbcDir, this);
    opcodes[0xE6] = std::bind(&CPU::IncDir, this);
    opcodes[0xE8] = std::bind(&CPU::InxImp, this);
    opcodes[0xE9] = std::bind(&CPU::SbcImm, this);
    opcodes[0xEB] = std::bind(&CPU::XbaImp, this);
    opcodes[0xF0] = std::bind(&CPU::BeqRel, this);
    opcodes[0xF4] = std::bind(&CPU::PeaImm, this);
    opcodes[0xFA] = std::bind(&CPU::PlxImp, this);
    opcodes[0xFB] = std::bind(&CPU::XceImp, this);
    opcodes[0xFC] = std::bind(&CPU::JsrAbx, this);

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

#undef printf

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

// #define printf(x, ...) 0

int CPU::TsbDir()
{
    uint16_t addr = d + ReadImm8();
    uint8_t data = Bus::Read8(addr);
    SetFlag(ZF, !(data & a.lo));
    data |= a.lo;
    Bus::Write8(addr, data);
    printf("tsb $%02x   ", addr-d);
    return 5;
}

int CPU::PhpImp()
{
    Push8(p);
    printf("php     ");
    return 3;
}

int CPU::OraImm()
{
    if (!GetFlag(MF))
    {
        uint16_t imm = ReadImm16();
        a.full |= imm;
        SetFlag(NF, (a.full >> 15) & 1);
        SetFlag(ZF, !a.full);
        printf("ora #$%04x ", imm);
        return 3;
    }
    else
    {
        uint8_t imm = ReadImm8();
        a.lo |= imm;
        SetFlag(NF, (a.lo >> 7) & 1);
        SetFlag(ZF, !a.lo);
        printf("ora #$%02x ", imm);
        return 2;
    }
}

int CPU::AslImp()
{
    if (!GetFlag(MF))
    {
        uint32_t result = a.full << 1;
        SetFlag(NF, (result >> 15) & 1);
        SetFlag(ZF, !result);
        SetFlag(CF, result > 0xFFFF);
        a.full = result;
        printf("asl     ");
        return 2;
    }
    else
    {
        uint16_t result = a.lo << 1;
        SetFlag(NF, (result >> 7) & 1);
        SetFlag(ZF, !result);
        SetFlag(CF, result > 0xFF);
        a.lo = result;
        printf("asl     ");
        return 2;
    }
}

int CPU::PhdImp()
{
    Push16(d);
    printf("phd     ");
    return 5;
}

int CPU::BplRel()
{
    int8_t rel = ReadImm8();
    int cycles = 2;
    if (!GetFlag(NF))
    {
        uint16_t new_pc = pc + rel;
        if ((new_pc & 0xff00) != (pc & 0xff00))
            cycles++;
        printf("bpl $%06x (t) ", pbr << 16 | new_pc);
        pc = new_pc;
        cycles++;
    }
    else
    {
        printf("bpl $%06x (n) ", pbr << 16 | (pc+rel));
    }

    return cycles;
}

int CPU::ClcImp()
{
    SetFlag(CF, false);

    printf("clc     ");

    return 2;
}

int CPU::IncImp()
{
    if (!GetFlag(MF))
    {
        uint16_t result = a.full + 1;
        SetFlag(NF, (result >> 15) & 1);
        SetFlag(ZF, !result);
        a.full = result;
        printf("inc     ");
        return 2;
    }
    else
    {
        uint8_t result = a.lo + 1;
        SetFlag(NF, (result >> 7) & 1);
        SetFlag(ZF, !result);
        a.lo = result;
        printf("inc     ");
        return 2;
    }
}

int CPU::TcsImp()
{
    sp = a.full;
    printf("tcs     ");
    return 2;
}

int CPU::JsrAbs()
{
    uint16_t new_pc = ReadImm16();
    Push16(pc-1);
    pc = new_pc;
    printf("jsr $%04x ", new_pc);
    return 6;
}

int CPU::JslLng()
{
    Push8(pbr);
    Push16(pc+2);

    uint16_t new_pc = ReadImm16();
    pbr = ReadImm8();

    pc = new_pc;

    printf("jsl $%06x ", pbr << 16 | pc);
    return 8;
}

int CPU::RolDir()
{
    uint16_t addr = d + ReadImm8();
    uint8_t data = Bus::Read8(addr);
    uint8_t old_carry = GetFlag(CF);
    SetFlag(CF, (data >> 7) & 1);
    data = (data << 1) | old_carry;
    SetFlag(NF, (data >> 7) & 1);
    SetFlag(ZF, !data);
    Bus::Write8(addr, data);
    printf("rol $%02x   ", addr-d);
    return 5;
}

int CPU::PlpImp()
{
    p = Pop8();
    if (e)
    {
        SetFlag(MF, 1);
        SetFlag(XBF, 1);
    }
    printf("plp     ");
    return 4;
}

int CPU::PlaImp()
{
    if (!GetFlag(MF))
    {
        a.full = Pop16();
        SetFlag(NF, (a.full >> 15) & 1);
        SetFlag(ZF, !a.full);
        printf("pla     ");
        return 5;
    }
    else
    {
        a.lo = Pop8();
        SetFlag(NF, (a.lo >> 7) & 1);
        SetFlag(ZF, !a.lo);
        printf("pla     ");
        return 4;
    }
}

int CPU::AdcImm()
{
    if (!GetFlag(MF))
    {
        uint16_t imm = ReadImm16();
        uint32_t result = a.full + imm + GetFlag(CF);
        SetFlag(NF, (result >> 15) & 1);
        SetFlag(ZF, !result);
        SetFlag(VF, ((a.full ^ result) & (imm ^ result) & 0x8000) != 0);
        SetFlag(CF, result > 0xFFFF);
        a.full = result;
        printf("adc #$%04x ", imm);
        return 3;
    }
    else
    {
        uint8_t imm = ReadImm8();
        uint16_t result = a.lo + imm + GetFlag(CF);
        SetFlag(NF, (result >> 7) & 1);
        SetFlag(ZF, !result);
        SetFlag(VF, ((a.lo ^ result) & (imm ^ result) & 0x80) != 0);
        SetFlag(CF, result > 0xFF);
        a.lo = result;
        printf("adc #$%02x ", imm);
        return 2;
    }
}

int CPU::RorImp()
{
    if (!GetFlag(MF))
    {
        bool old_cf = GetFlag(CF);
        SetFlag(CF, a.full & 1);
        a.full = (a.full >> 1) | (old_cf << 15);
        SetFlag(NF, (a.full >> 15) & 1);
        SetFlag(ZF, !a.full);
        printf("ror     ");
        return 2;
    }
    else
    {
        bool old_cf = GetFlag(CF);
        SetFlag(CF, a.lo & 1);
        a.lo = (a.lo >> 1) | (old_cf << 7);
        SetFlag(NF, (a.lo >> 7) & 1);
        SetFlag(ZF, !a.lo);
        printf("ror     ");
        return 2;
    }
}

int CPU::RtlImp()
{
    uint16_t new_pc = Pop16();
    pbr = Pop8();
    pc = new_pc + 1;

    printf("rtl     ");
    return 6;
}

int CPU::JmpAbP()
{
    uint16_t ptr_addr = ReadImm16();
    uint16_t new_pc = Bus::Read16(ptr_addr);

    pc = new_pc;

    printf("jmp ($%04x) ", ptr_addr);
    return 5;
}

int CPU::StzDrx()
{
    uint16_t addr = GetDirXAddr();

    if (!GetFlag(MF))
    {
        Bus::Write16(addr, 0);
        printf("stz $%02x,x ", addr-d-x.lo);
        return 5;
    }
    else
    {
        Bus::Write8(addr, 0);
        printf("stz $%02x,x ", addr-d-x.lo);
        return 4;
    }
}

int CPU::SeiImp()
{
    SetFlag(IF, true);

    printf("sei     ");

    return 2;
}

int CPU::PlyImp()
{
    if (!GetFlag(MF))
    {
        y.full = Pop16();
        SetFlag(NF, (y.full >> 15) & 1);
        SetFlag(ZF, !y.full);
        printf("ply     ");
        return 5;
    }
    else
    {
        y.lo = Pop8();
        SetFlag(NF, (y.lo >> 7) & 1);
        SetFlag(ZF, !y.lo);
        printf("ply     ");
        return 4;
    }
}

int CPU::TdcImp()
{
    a.full = d;
    SetFlag(NF, (d >> 15) & 1);
    SetFlag(ZF, !d);

    printf("tdc     ");

    return 2;
}

int CPU::BraRel()
{
    int8_t rel = ReadImm8();
    int cycles = 2;

    uint16_t new_pc = pc + rel;
    if ((new_pc & 0xff00) != (pc & 0xff00))
        cycles++;
    printf("bra $%06x (t) ", pbr << 16 | new_pc);
    pc = new_pc;
    cycles++;

    return cycles;
}

int CPU::LdaDir()
{
    uint16_t addr = d + ReadImm8();

    if (!GetFlag(MF))
    {
        a.full = Bus::Read16(addr);
        SetFlag(NF, (a.full >> 15) & 1);
        SetFlag(ZF, !a.full);
        printf("lda $%02x   ", addr-d);
        return 5;
    }
    else
    {
        a.lo = Bus::Read8(addr);
        SetFlag(NF, (a.lo >> 7) & 1);
        SetFlag(ZF, !a.lo);
        printf("lda $%02x   ", addr-d);
        return 4;
    }
}

int CPU::LdaDPL()
{
    uint16_t ptr_addr = d + ReadImm8();
    uint32_t addr = Bus::Read16(ptr_addr);
    addr |= (uint32_t)Bus::Read8(ptr_addr+2) << 16;

    if (!GetFlag(MF))
    {
        a.full = Bus::Read16(addr);
        SetFlag(NF, (a.full >> 15) & 1);
        SetFlag(ZF, !a.full);
        printf("lda [$%02x] ", ptr_addr-d);
        return 6;
    }
    else
    {
        a.lo = Bus::Read8(addr);
        SetFlag(NF, (a.lo >> 7) & 1);
        SetFlag(ZF, !a.lo);
        printf("lda [$%02x] ", ptr_addr-d);
        return 5;
    }
}

int CPU::TayImp()
{
    if (!GetFlag(MF))
    {
        y.full = a.full;
        SetFlag(NF, (y.full >> 15) & 1);
        SetFlag(ZF, !y.full);
        printf("tay     ");
        return 2;
    }
    else
    {
        y.lo = a.lo;
        SetFlag(NF, (y.lo >> 7) & 1);
        SetFlag(ZF, !y.lo);
        printf("tay     ");
        return 2;
    }
}

int CPU::LdaImm()
{
    if (!GetFlag(MF))
    {
        uint16_t imm = ReadImm16();
        a.full = imm;
        SetFlag(NF, (a.full >> 15) & 1);
        SetFlag(ZF, !a.full);
        printf("lda #$%04x ", imm);
        return 3;
    }
    else
    {
        uint8_t imm = ReadImm8();
        a.lo = imm;
        SetFlag(NF, (a.lo >> 7) & 1);
        SetFlag(ZF, !a.lo);
        printf("lda #$%02x ", imm);
        return 2;
    }
}

int CPU::TaxImp()
{
    if (!GetFlag(MF))
    {
        x.full = a.full;
        SetFlag(NF, (x.full >> 15) & 1);
        SetFlag(ZF, !x.full);
        printf("tax     ");
        return 2;
    }
    else
    {
        x.lo = a.lo;
        SetFlag(NF, (x.lo >> 7) & 1);
        SetFlag(ZF, !x.lo);
        printf("tax     ");
        return 2;
    }
}

int CPU::PlbImp()
{
    dbr = Pop8();
    printf("plb     ");
    return 4;
}

int CPU::LdaAbs()
{
    uint16_t addr = ReadImm16();
    if (!GetFlag(MF))
    {
        a.full = Bus::Read16(dbr << 16 | addr);
        SetFlag(NF, (a.full >> 15) & 1);
        SetFlag(ZF, !a.full);
        printf("lda $%04x ", addr);
        return 4;
    }
    else
    {
        a.lo = Bus::Read8(dbr << 16 | addr);
        SetFlag(NF, (a.lo >> 7) & 1);
        SetFlag(ZF, !a.lo);
        printf("lda $%04x ", addr);
        return 3;
    }
}

int CPU::LdxAbs()
{
    uint16_t addr = ReadImm16();
    if (!GetFlag(XBF))
    {
        x.full = Bus::Read16(dbr << 16 | addr);
        SetFlag(NF, (x.full >> 15) & 1);
        SetFlag(ZF, !x.full);
        printf("ldx $%04x ", addr);
        return 4;
    }
    else
    {
        x.full = Bus::Read8(dbr << 16 | addr);
        SetFlag(NF, (x.lo >> 7) & 1);
        SetFlag(ZF, !x.lo);
        printf("ldx $%04x ", addr);
        return 3;
    }
}

int CPU::LdaLnX()
{
    uint32_t addr = ReadImm16();
    addr |= (uint32_t)ReadImm8() << 16;

    if (!GetFlag(MF))
    {
        a.full = Bus::Read16(addr + x.full);
        SetFlag(NF, (a.full >> 15) & 1);
        SetFlag(ZF, !a.full);
        printf("lda $%06x,x ", addr);
        return 5;
    }
    else
    {
        a.lo = Bus::Read8(addr + x.full);
        SetFlag(NF, (a.lo >> 7) & 1);
        SetFlag(ZF, !a.lo);
        printf("lda $%06x,x ", addr);
        return 4;
    }
}

int CPU::CpyImm()
{
    if (!GetFlag(XBF))
    {
        uint16_t imm = ReadImm16();
        uint16_t result = y.full - imm;
        SetFlag(NF, (result >> 15) & 1);
        SetFlag(ZF, !result);
        SetFlag(CF, y.full >= imm);
        printf("cpy #$%04x ", imm);
        return 3;
    }
    else
    {
        uint8_t imm = ReadImm8();
        uint8_t result = y.lo - imm;
        SetFlag(NF, (result >> 7) & 1);
        SetFlag(ZF, !result);
        SetFlag(CF, y.lo >= imm);
        printf("cpy #$%02x ", imm);
        return 2;
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

int CPU::CpyDir()
{
    uint16_t addr = d + ReadImm8();
    if (!GetFlag(MF))
    {
        uint16_t data = Bus::Read16(addr);
        uint16_t result = y.full - data;
        SetFlag(NF, (result >> 15) & 1);
        SetFlag(ZF, !result);
        SetFlag(CF, y.full >= data);
        printf("cpy $%02x   ", addr-d);
        return 5;
    }
    else
    {
        uint8_t data = Bus::Read8(addr);
        uint8_t result = y.lo - data;
        SetFlag(NF, (result >> 7) & 1);
        SetFlag(ZF, !result);
        SetFlag(CF, y.lo >= data);
        printf("cpy $%02x   ", addr-d);
        return 4;
    }
}

int CPU::DecDir()
{
    uint16_t addr = d + ReadImm8();

    if (!GetFlag(MF))
    {
        uint16_t result = Bus::Read16(addr) - 1;
        SetFlag(NF, (result >> 15) & 1);
        SetFlag(ZF, !result);
        Bus::Write16(addr, result);
        printf("dec $%02x   ", addr-d);
        return 5;
    }
    else
    {
        uint8_t result = Bus::Read8(addr) - 1;
        SetFlag(NF, (result >> 7) & 1);
        SetFlag(ZF, !result);
        Bus::Write8(addr, result);
        printf("dec $%02x   ", addr-d);
        return 4;
    }
}

int CPU::InyImp()
{
    if (!GetFlag(MF))
    {
        y.full++;
        SetFlag(NF, (y.full >> 15) & 1);
        SetFlag(ZF, !y.full);
        printf("iny     ");
        return 2;
    }
    else
    {
        y.lo++;
        SetFlag(NF, (y.lo >> 7) & 1);
        SetFlag(ZF, !y.lo);
        printf("iny     ");
        return 2;
    }
}

int CPU::CmpImm()
{
    if (!GetFlag(MF))
    {
        uint16_t imm = ReadImm16();
        uint16_t result = a.full - imm;
        SetFlag(NF, (result >> 15) & 1);
        SetFlag(ZF, !result);
        SetFlag(CF, a.full < imm);
        printf("cmp #$%04x ", imm);
        return 3;
    }
    else
    {
        uint8_t imm = ReadImm8();
        uint8_t result = a.lo - imm;
        SetFlag(NF, (result >> 7) & 1);
        SetFlag(ZF, !result);
        SetFlag(CF, a.lo < imm);
        printf("cmp #$%02x ", imm);
        return 2;
    }
}

int CPU::DexImp()
{
    if (!GetFlag(XBF))
    {
        x.full--;
        SetFlag(NF, (x.full >> 15) & 1);
        SetFlag(ZF, !x.full);
        printf("dex     ");
        return 2;
    }
    else
    {
        x.lo--;
        SetFlag(NF, (x.lo >> 7) & 1);
        SetFlag(ZF, !x.lo);
        printf("dex     ");
        return 2;
    }
}

int CPU::CmpAbs()
{
    uint16_t addr = ReadImm16();
    if (!GetFlag(MF))
    {
        uint16_t data = Bus::Read16(dbr << 16 | addr);
        uint16_t result = a.full - data;
        SetFlag(NF, (result >> 15) & 1);
        SetFlag(ZF, !result);
        SetFlag(CF, a.full < data);
        printf("cmp $%04x ", addr);
        return 5;
    }
    else
    {
        uint8_t data = Bus::Read8(dbr << 16 | addr);
        uint8_t result = a.lo - data;
        SetFlag(NF, (result >> 7) & 1);
        SetFlag(ZF, !result);
        SetFlag(CF, a.lo < data);
        printf("cmp $%04x ", addr);
        return 4;
    }
}

int CPU::BneRel()
{
    int8_t rel = ReadImm8();
    int cycles = 2;
    if (!GetFlag(ZF))
    {
        uint16_t new_pc = pc + rel;
        if ((new_pc & 0xff00) != (pc & 0xff00))
            cycles++;
        printf("bne $%06x (t) ", pbr << 16 | new_pc);
        pc = new_pc;
        cycles++;
    }
    else
    {
        printf("bne $%06x (n) ", pbr << 16 | (pc+rel));
    }

    return cycles;
}

int CPU::CldImp()
{
    SetFlag(DF, false);
    printf("cld     ");
    return 2;
}

int CPU::PhxImp()
{
    if (!GetFlag(XBF))
    {
        Push16(x.full);
        printf("phx     ");
        return 4;
    }
    else
    {
        Push8(x.lo);
        printf("phx     ");
        return 3;
    }
}

int CPU::CpxImm()
{
    if (!GetFlag(XBF))
    {
        uint16_t imm = ReadImm16();
        uint16_t result = x.full - imm;
        SetFlag(NF, (result >> 15) & 1);
        SetFlag(ZF, !result);
        SetFlag(CF, x.full >= imm);
        printf("cpx #$%04x ", imm);
        return 3;
    }
    else
    {
        uint8_t imm = ReadImm8();
        uint8_t result = x.lo - imm;
        SetFlag(NF, (result >> 7) & 1);
        SetFlag(ZF, !result);
        SetFlag(CF, x.lo >= imm);
        printf("cpx #$%02x ", imm);
        return 2;
    }
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

int CPU::SbcDir()
{
    uint16_t addr = d + ReadImm8();

    if (!GetFlag(MF))
    {
        uint16_t imm = Bus::Read16(addr);
        uint32_t result = a.full - imm - 1 + GetFlag(CF);
        SetFlag(NF, (result >> 15) & 1);
        SetFlag(ZF, !result);
        SetFlag(VF, ((a.full ^ result) & (a.full ^ imm) & 0x8000) != 0);
        SetFlag(CF, result > 0xFFFF);
        a.full = result;
        printf("sbc $%02x   ", addr-d);
        return 5;
    }
    else
    {
        uint8_t imm = Bus::Read8(addr);
        uint16_t result = a.lo - imm - 1 + GetFlag(CF);
        SetFlag(NF, (result >> 7) & 1);
        SetFlag(ZF, !result);
        SetFlag(VF, ((a.lo ^ result) & (a.lo ^ imm) & 0x80) != 0);
        SetFlag(CF, result > 0xFF);
        a.lo = result;
        printf("sbc $%02x   ", addr-d);
        return 4;
    }
}

int CPU::IncDir()
{
    uint16_t addr = d + ReadImm8();

    if (!GetFlag(MF))
    {
        uint16_t result = Bus::Read16(addr) + 1;
        Bus::Write16(addr, result);
        SetFlag(NF, (result >> 15) & 1);
        SetFlag(ZF, !result);
        printf("inc $%02x   ", addr-d);
        return 5;
    }
    else
    {
        uint8_t result = Bus::Read8(addr) + 1;
        Bus::Write8(addr, result);
        SetFlag(NF, (result >> 7) & 1);
        SetFlag(ZF, !result);
        printf("inc $%02x   ", addr-d);
        return 4;
    }
}

int CPU::InxImp()
{
    if (!GetFlag(XBF))
    {
        x.full++;
        SetFlag(NF, (x.full >> 15) & 1);
        SetFlag(ZF, !x.full);
        printf("inx     ");
        return 2;
    }
    else
    {
        x.lo++;
        SetFlag(NF, (x.lo >> 7) & 1);
        SetFlag(ZF, !x.lo);
        printf("inx     ");
        return 2;
    }
}

int CPU::SbcImm()
{
    if (!GetFlag(MF))
    {
        uint16_t imm = ReadImm16();
        uint32_t result = a.full - imm - 1 + GetFlag(CF);
        SetFlag(NF, (result >> 15) & 1);
        SetFlag(ZF, !result);
        SetFlag(VF, ((a.full ^ result) & (a.full ^ imm) & 0x8000) != 0);
        SetFlag(CF, result <= 0xFFFF);
        a.full = result;
        printf("sbc #$%04x ", imm);
        return 3;
    }
    else
    {
        uint8_t imm = ReadImm8();
        uint16_t result = a.lo - imm - 1 + GetFlag(CF);
        SetFlag(NF, (result >> 7) & 1);
        SetFlag(ZF, !result);
        SetFlag(VF, ((a.lo ^ result) & (a.lo ^ imm) & 0x80) != 0);
        SetFlag(CF, result <= 0xFF);
        a.lo = result;
        printf("sbc #$%02x ", imm);
        return 2;
    }
}

int CPU::XbaImp()
{
    uint8_t temp = a.lo;
    a.lo = a.hi;
    a.hi = temp;
    SetFlag(NF, (a.lo >> 7) & 1);
    SetFlag(ZF, !a.lo);
    printf("xba     ");
    return 3;
}

int CPU::BeqRel()
{
    int8_t rel = ReadImm8();
    int cycles = 2;
    if (GetFlag(ZF))
    {
        uint16_t new_pc = pc + rel;
        if ((new_pc & 0xff00) != (pc & 0xff00))
            cycles++;
        printf("beq $%06x (t) ", pbr << 16 | new_pc);
        pc = new_pc;
        cycles++;
    }
    else
    {
        printf("beq $%06x (n) ", pbr << 16 | (pc+rel));
    }

    return cycles;
}

int CPU::PeaImm()
{
    uint16_t imm = ReadImm16();
    Push16(imm);
    printf("pea #$%04x ", imm);
    return 5;
}

int CPU::PlxImp()
{
    if (!GetFlag(XBF))
    {
        x.full = Pop16();
        SetFlag(NF, (x.full >> 15) & 1);
        SetFlag(ZF, !x.full);
        printf("plx     ");
        return 5;
    }
    else
    {
        x.lo = Pop8();
        SetFlag(NF, (x.lo >> 7) & 1);
        SetFlag(ZF, !x.lo);
        printf("plx     ");
        return 4;
    }
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

int CPU::JsrAbx()
{
    uint16_t addr = ReadImm16() + x.full;
    uint16_t new_pc = Bus::Read16(pbr << 16 | addr);

    Push16(pc-1);

    pc = new_pc;

    printf("jsr ($%04x,x) ", addr);
    return 8;
}

int CPU::BitImm()
{
    if (!GetFlag(MF))
    {
        uint16_t imm = ReadImm16();
        uint16_t result = a.full & imm;
        SetFlag(ZF, result == 0);
        printf("bit #$%04x ", imm);
        return 3;
    }
    else
    {
        uint8_t imm = ReadImm8();
        uint8_t result = a.lo & imm;
        SetFlag(ZF, result == 0);
        printf("bit #$%02x ", imm);
        return 2;
    }
}

int CPU::TxaImp()
{
    if (!GetFlag(MF))
    {
        a.full = x.full;
        SetFlag(NF, (a.full >> 15) & 1);
        SetFlag(ZF, !a.full);
        printf("txa     ");
        return 2;
    }
    else
    {
        a.lo = x.lo;
        SetFlag(NF, (a.lo >> 7) & 1);
        SetFlag(ZF, !a.lo);
        printf("txa     ");
        return 2;
    }
}

int CPU::PhbImp()
{
    Push8(dbr);
    printf("phb     ");
    return 3;
}

int CPU::StyAbs()
{
    uint16_t addr = ReadImm16();
    if (!GetFlag(XBF))
    {
        Bus::Write16(dbr << 16 | addr, y.full);
        printf("sty $%04x ", addr);
        return 4;
    }
    else
    {
        Bus::Write8(dbr << 16 | addr, y.lo);
        printf("sty $%04x ", addr);
        return 3;
    }
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

int CPU::StxAbs()
{
    std::string disasm;
    int cycles;
    if (!GetFlag(XBF))
    {
        SetAbs16(disasm, x.full);
        cycles = 5;
    }
    else
    {
        SetAbs8(disasm, x.lo);
        cycles = 4;
    }
    printf("stx %s ", disasm.c_str());
    return cycles;
}

int CPU::BccRel()
{
    int8_t rel = ReadImm8();
    int cycles = 2;
    if (!GetFlag(CF))
    {
        uint16_t new_pc = pc + rel;
        if ((new_pc & 0xff00) != (pc & 0xff00))
            cycles++;
        printf("bcc $%06x (t) ", pbr << 16 | new_pc);
        pc = new_pc;
        cycles++;
    }
    else
    {
        printf("bcc $%06x (n) ", pbr << 16 | (pc+rel));
    }

    return cycles;
}

int CPU::StaDrp()
{
    uint8_t ptr_addr = d + ReadImm8();
    uint16_t addr = Bus::Read16(ptr_addr);

    if (!GetFlag(MF))
    {
        Bus::Write16(dbr << 16 | addr, a.full);
        printf("sta [$%02x] ", ptr_addr-d);
        return 6;
    }
    else
    {
        Bus::Write8(dbr << 16 | addr, a.lo);
        printf("sta [$%02x] ", ptr_addr-d);
        return 5;
    }
}

int CPU::TyaImp()
{
    if (!GetFlag(MF))
    {
        a.full = y.full;
        SetFlag(NF, (a.full >> 15) & 1);
        SetFlag(ZF, !a.full);
        printf("tya     ");
        return 2;
    }
    else
    {
        a.lo = y.lo;
        SetFlag(NF, (a.lo >> 7) & 1);
        SetFlag(ZF, !a.lo);
        printf("tya     ");
        return 2;
    }
}

int CPU::StaAbY()
{
    uint16_t addr = ReadImm16() + y.full;
    if (!GetFlag(MF))
    {
        Bus::Write16(dbr << 16 | addr, a.full);
        printf("sta $%04x,y ", addr-y.full);
        return 5;
    }
    else
    {
        Bus::Write8(dbr << 16 | addr, a.lo);
        printf("sta $%04x,y ", addr-y.full);
        return 4;
    }
}

int CPU::PhyImp()
{
    if (!GetFlag(XBF))
    {
        Push16(y.full);
        printf("phy     ");
        return 4;
    }
    else
    {
        Push8(y.lo);
        printf("phy     ");
        return 3;
    }
}

int CPU::TcdImp()
{
    d = a.full;

    SetFlag(NF, (d >> 15) & 1);
    SetFlag(ZF, !d);

    printf("tcd     ");
    return 2;
}

int CPU::JmpLng()
{
    uint16_t new_pc = ReadImm16();
    pbr = ReadImm8();
    pc = new_pc;

    printf("jmp $%06x ", pbr << 16 | pc);
    return 4;
}

int CPU::RtsImp()
{
    pc = Pop16() + 1;
    printf("rts     ");
    return 6;
}

int CPU::StzDir()
{
    uint16_t addr = d + ReadImm8();

    if (!GetFlag(MF))
    {
        Bus::Write16(addr, 0);
        printf("stz $%02x   ", addr-d);
        return 5;
    }
    else
    {
        Bus::Write8(addr, 0);
        printf("stz $%02x   ", addr-d);
        return 4;
    }
}

int CPU::AdcDir()
{
    uint16_t addr = d + ReadImm8();

    if (!GetFlag(MF))
    {
        uint16_t imm = Bus::Read16(addr);
        uint32_t result = a.full + imm + GetFlag(CF);
        SetFlag(NF, (result >> 15) & 1);
        SetFlag(ZF, !result);
        SetFlag(VF, ((a.full ^ result) & (imm ^ result) & 0x8000) != 0);
        SetFlag(CF, result > 0xFFFF);
        a.full = result;
        printf("adc $%02x   ", addr-d);
        return 5;
    }
    else
    {
        uint8_t imm = Bus::Read8(addr);
        uint16_t result = a.lo + imm + GetFlag(CF);
        SetFlag(NF, (result >> 7) & 1);
        SetFlag(ZF, !result);
        SetFlag(VF, ((a.lo ^ result) & (imm ^ result) & 0x80) != 0);
        SetFlag(CF, result > 0xFF);
        a.lo = result;
        printf("adc $%02x   ", addr-d);
        return 4;
    }
}

int CPU::AdcDrP()
{
    uint8_t ptr_addr = d + ReadImm8();
    uint32_t addr = Bus::Read16(ptr_addr);
    addr |= (uint32_t)Bus::Read8(ptr_addr+2) << 16;

    if (!GetFlag(MF))
    {
        uint16_t imm = Bus::Read16(addr);
        uint32_t result = a.full + imm + GetFlag(CF);
        SetFlag(NF, (result >> 15) & 1);
        SetFlag(ZF, !result);
        SetFlag(VF, ((a.full ^ result) & (imm ^ result) & 0x8000) != 0);
        SetFlag(CF, result > 0xFFFF);
        a.full = result;
        printf("adc [$%02x] ", ptr_addr-d);
        return 6;
    }
    else
    {
        uint8_t imm = Bus::Read8(addr);
        uint16_t result = a.lo + imm + GetFlag(CF);
        SetFlag(NF, (result >> 7) & 1);
        SetFlag(ZF, !result);
        SetFlag(VF, ((a.lo ^ result) & (imm ^ result) & 0x80) != 0);
        SetFlag(CF, result > 0xFF);
        a.lo = result;
        printf("adc [$%02x] ", ptr_addr-d);
        return 5;
    }
}

int CPU::RtiImp()
{
    p = Pop8();
    pc = Pop16();
    pbr = Pop8();
    printf("rti     ");
    return 6;
}

int CPU::PhaImp()
{
    if (!GetFlag(MF))
    {
        Push16(a.full);
        printf("pha     ");
        return 4;
    }
    else
    {
        Push8(a.lo);
        printf("pha     ");
        return 3;
    }
}

int CPU::EorImm()
{
    if (!GetFlag(MF))
    {
        uint16_t imm = ReadImm16();
        a.full ^= imm;
        SetFlag(NF, (a.full >> 15) & 1);
        SetFlag(ZF, !a.full);
        printf("eor #$%04x ", imm);
        return 3;
    }
    else
    {
        uint8_t imm = ReadImm8();
        a.lo ^= imm;
        SetFlag(NF, (a.lo >> 7) & 1);
        SetFlag(ZF, !a.lo);
        printf("eor #$%02x ", imm);
        return 2;
    }
}

int CPU::LsrImp()
{
    if (!GetFlag(MF))
    {
        SetFlag(CF, a.full & 1);
        a.full >>= 1;
        SetFlag(NF, (a.full >> 15) & 1);
        SetFlag(ZF, !a.full);
        printf("lsr     ");
        return 2;
    }
    else
    {
        SetFlag(CF, a.lo & 1);
        a.lo >>= 1;
        SetFlag(NF, (a.lo >> 7) & 1);
        SetFlag(ZF, !a.lo);
        printf("lsr     ");
        return 2;
    }
}

int CPU::PhkImp()
{
    Push8(pbr);
    printf("phk     ");
    return 3;
}

int CPU::JmpAbs()
{
    uint16_t addr = ReadImm16();
    pc = addr;
    printf("jmp $%04x ", addr);
    return 3;
}

int CPU::TxyImp()
{
    if (!GetFlag(XBF))
    {
        y.full = x.full;
        SetFlag(NF, (y.full >> 15) & 1);
        SetFlag(ZF, !y.full);
        printf("txy     ");
        return 2;
    }
    else
    {
        y.lo = x.lo;
        SetFlag(NF, (y.lo >> 7) & 1);
        SetFlag(ZF, !y.lo);
        printf("txy     ");
        return 2;
    }
}

int CPU::StzAbs()
{
    std::string disasm;
    int cycles;
    if (!GetFlag(MF))
    {
        SetAbs16(disasm, 0);
        cycles = 5;
    }
    else
    {
        SetAbs8(disasm, 0);
        cycles = 4;
    }
    printf("stz %s ", disasm.c_str());
    return cycles;
}

int CPU::StaAbx()
{
    uint16_t addr = ReadImm16() + x.full;
    if (!GetFlag(MF))
    {
        Bus::Write16(dbr << 16 | addr, a.full);
        printf("sta $%04x,x ", addr-x.full);
        return 5;
    }
    else
    {
        Bus::Write8(dbr << 16 | addr, a.lo);
        printf("sta $%04x,x ", addr-x.full);
        return 4;
    }
}

int CPU::StzAbx()
{
    uint16_t addr = GetAbsXAddr();
    if (!GetFlag(MF))
    {
        Bus::Write16(addr, 0);
        printf("stz $%04x,x ", (addr&0xFFFF)-x.full);
        return 5;
    }
    else
    {
        Bus::Write8(addr, 0);
        printf("stz $%04x,x ", (addr&0xFFFF)-x.full);
        return 4;
    }
}

int CPU::LdyImm()
{
    if (!GetFlag(XBF))
    {
        y.full = ReadImm16();
        SetFlag(NF, (y.full >> 15) & 1);
        SetFlag(ZF, !y.full);
        printf("ldy #$%04x ", y.full);
        return 3;
    }
    else
    {
        y.full = ReadImm8();
        SetFlag(NF, (y.lo >> 7) & 1);
        SetFlag(ZF, !y.lo);
        printf("ldy #$%02x ", y.lo);
        return 2;
    }
}

int CPU::LdxImm()
{
    if (!GetFlag(XBF))
    {
        x.full = ReadImm16();
        SetFlag(NF, (x.full >> 15) & 1);
        SetFlag(ZF, !x.full);
        printf("ldx #$%04x ", x.full);
        return 3;
    }
    else
    {
        x.full = ReadImm8();
        SetFlag(NF, (x.lo >> 7) & 1);
        SetFlag(ZF, !x.lo);
        printf("ldx #$%02x ", x.lo);
        return 2;
    }
}

int CPU::LdyDir()
{
    uint16_t addr = d + ReadImm8();

    if (!GetFlag(XBF))
    {
        y.full = Bus::Read16(addr);
        SetFlag(NF, (y.full >> 15) & 1);
        SetFlag(ZF, !y.full);
        printf("ldy $%02x   ", addr-d);
        return 5;
    }
    else
    {
        y.lo = Bus::Read8(addr);
        SetFlag(NF, (y.lo >> 7) & 1);
        SetFlag(ZF, !y.lo);
        printf("ldy $%02x   ", addr-d);
        return 4;
    }
}

int CPU::AndImm()
{
    if (!GetFlag(MF))
    {
        uint16_t imm = ReadImm16();
        a.full &= imm;
        SetFlag(NF, (a.full >> 15) & 1);
        SetFlag(ZF, !a.full);
        printf("and #$%04x ", imm);
        return 3;
    }
    else
    {
        uint8_t imm = ReadImm8();
        a.lo &= imm;
        SetFlag(NF, (a.lo >> 7) & 1);
        SetFlag(ZF, !a.lo);
        printf("and #$%02x ", imm);
        return 2;
    }
}

int CPU::RolImp()
{
    if (!GetFlag(MF))
    {
        uint16_t temp = a.full;
        a.full <<= 1;
        a.full |= GetFlag(CF);
        SetFlag(CF, (temp >> 15) & 1);
        SetFlag(NF, (a.full >> 15) & 1);
        SetFlag(ZF, !a.full);
        printf("rol     ");
        return 2;
    }
    else
    {
        uint8_t temp = a.lo;
        a.lo <<= 1;
        a.lo |= GetFlag(CF);
        SetFlag(CF, (temp >> 7) & 1);
        SetFlag(NF, (a.lo >> 7) & 1);
        SetFlag(ZF, !a.lo);
        printf("rol     ");
        return 2;
    }
}

int CPU::PldImp()
{
    d = Pop16();
    SetFlag(NF, (d >> 15) & 1);
    SetFlag(ZF, !d);
    printf("pld     ");
    return 5;
}

int CPU::BitAbs()
{
    uint16_t abs = ReadImm16();
    if (!GetFlag(MF))
    {
        uint16_t data = Bus::Read16(dbr << 16 | abs);
        SetFlag(NF, (data >> 15) & 1);
        SetFlag(VF, (data >> 14) & 1);
        SetFlag(ZF, !(data & a.full));
        printf("bit $%04x ", abs);
        return 5;
    }
    else
    {
        uint8_t data = Bus::Read8(dbr << 16 | abs);
        SetFlag(NF, (data >> 7) & 1);
        SetFlag(VF, (data >> 6) & 1);
        SetFlag(ZF, !(data & a.lo));
        printf("bit $%04x ", abs);
        return 4;
    }
}

int CPU::BmiRel()
{
    int8_t rel = ReadImm8();
    int cycles = 2;
    if (GetFlag(NF))
    {
        uint16_t new_pc = pc + rel;
        if ((new_pc & 0xff00) != (pc & 0xff00))
            cycles++;
        printf("bmi $%06x (t) ", pbr << 16 | new_pc);
        pc = new_pc;
        cycles++;
    }
    else
    {
        printf("bmi $%06x (n) ", pbr << 16 | (pc+rel));
    }

    return cycles;
}

int CPU::SecImp()
{
    SetFlag(CF, true);
    printf("sec     ");
    return 2;
}

int CPU::DecAcc()
{
    if (!GetFlag(MF))
    {
        a.full--;
        SetFlag(NF, (a.full >> 15) & 1);
        SetFlag(ZF, !a.full);
        printf("dec     ");
        return 2;
    }
    else
    {
        a.lo--;
        SetFlag(NF, (a.lo >> 7) & 1);
        SetFlag(ZF, !a.lo);
        printf("dec     ");
        return 2;
    }
}

int CPU::BcsRel()
{
    int8_t rel = ReadImm8();
    int cycles = 2;
    if (GetFlag(CF))
    {
        uint16_t new_pc = pc + rel;
        if ((new_pc & 0xff00) != (pc & 0xff00))
            cycles++;
        printf("bcs $%06x (t) ", pbr << 16 | new_pc);
        pc = new_pc;
        cycles++;
    }
    else
    {
        printf("bcs $%06x (n) ", pbr << 16 | (pc+rel));
    }

    return cycles;
}

int CPU::LdaDpt()
{
    uint16_t ptr_addr = ReadImm8() + d;
    uint32_t addr = dbr << 16 | Bus::Read16(ptr_addr);

    if (!GetFlag(MF))
    {
        a.full = Bus::Read16(addr);
        SetFlag(NF, (a.full >> 15) & 1);
        SetFlag(ZF, !a.full);
        printf("lda ($%02x) ", ptr_addr-d);
        return 6;
    }
    else
    {
        a.lo = Bus::Read8(addr);
        SetFlag(NF, (a.lo >> 7) & 1);
        SetFlag(ZF, !a.lo);
        printf("lda ($%02x) ", ptr_addr-d);
        return 5;
    }
}

// [dir],y
int CPU::LdaDPY()
{
    uint16_t ptr_addr = ReadImm8() + d;
    uint32_t addr = Bus::Read16(ptr_addr);
    addr |= (uint32_t)Bus::Read8(ptr_addr+2) << 16;
    addr += y.full;

    if (!GetFlag(MF))
    {
        a.full = Bus::Read16(addr);
        SetFlag(NF, (a.full >> 15) & 1);
        SetFlag(ZF, !a.full);
        printf("lda [$%02x],y ", ptr_addr-d);
        return 6;
    }
    else
    {
        a.lo = Bus::Read8(addr);
        SetFlag(NF, (a.lo >> 7) & 1);
        SetFlag(ZF, !a.lo);
        printf("lda [$%02x],y ", ptr_addr-d);
        return 5;
    }
}

int CPU::TyxImp()
{
    if (!GetFlag(XBF))
    {
        x.full = y.full;
        SetFlag(NF, (x.full >> 15) & 1);
        SetFlag(ZF, !x.full);
        printf("tyx     ");
        return 2;
    }
    else
    {
        x.lo = y.lo;
        SetFlag(NF, (x.lo >> 7) & 1);
        SetFlag(ZF, !x.lo);
        printf("tyx     ");
        return 2;
    }
}

int CPU::LdaAbx()
{
    uint16_t addr = ReadImm16() + x.full;
    if (!GetFlag(MF))
    {
        a.full = Bus::Read16(dbr << 16 | addr);
        SetFlag(NF, (a.full >> 15) & 1);
        SetFlag(ZF, !a.full);
        printf("lda $%04x,x ", addr-x.full);
        return 5;
    }
    else
    {
        a.lo = Bus::Read8(dbr << 16 | addr);
        SetFlag(NF, (a.lo >> 7) & 1);
        SetFlag(ZF, !a.lo);
        printf("lda $%04x,x ", addr-x.full);
        return 4;
    }
}

int CPU::StyDir()
{
    uint16_t addr = d + ReadImm8();
    if (!GetFlag(XBF))
    {
        Bus::Write16(addr, y.full);
        printf("sty $%02x ", addr-d);
        return 4;
    }
    else
    {
        Bus::Write8(addr, y.lo);
        printf("sty $%02x ", addr-d);
        return 3;
    }
}

int CPU::StaDir()
{
    uint16_t addr = d + ReadImm8();
    if (!GetFlag(MF))
    {
        Bus::Write16(addr, a.full);
        printf("sta $%02x ", addr-d);
        return 4;
    }
    else
    {
        Bus::Write8(addr, a.lo);
        printf("sta $%02x ", addr-d);
        return 3;
    }
}

int CPU::StxDir()
{
    uint16_t addr = d + ReadImm8();
    if (!GetFlag(XBF))
    {
        Bus::Write16(addr, x.full);
        printf("stx $%02x ", addr-d);
        return 4;
    }
    else
    {
        Bus::Write8(addr, x.lo);
        printf("stx $%02x ", addr-d);
        return 3;
    }
}

int CPU::DeyImp()
{
    if (!GetFlag(XBF))
    {
        y.full--;
        SetFlag(NF, (y.full >> 15) & 1);
        SetFlag(ZF, !y.full);
        printf("dey     ");
        return 2;
    }
    else
    {
        y.lo--;
        SetFlag(NF, (y.lo >> 7) & 1);
        SetFlag(ZF, !y.lo);
        printf("dey     ");
        return 2;
    }
}
