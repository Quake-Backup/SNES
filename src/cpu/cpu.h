#pragma once

#include <cstdint>
#include <functional>
#include <unordered_map>
#include <string>

union Register
{
    uint8_t lo;
    uint16_t full;
};

class CPU
{
private:
    uint16_t pc;
    uint16_t sp;
    Register a, x, y;
    uint8_t p;

    uint8_t dbr;
    uint8_t pbr;

    uint16_t d;

    enum Flags
    {
        NF = (1 << 7),
        VF = (1 << 6),
        MF = (1 << 5),
        XBF = (1 << 4),
        DF = (1 << 3),
        IF = (1 << 2),
        ZF = (1 << 1),
        CF = (1 << 0),
    };

    uint8_t ReadImm8();
    uint16_t ReadImm16();

    void SetAbs8(std::string& disasm, uint8_t data);
    void SetAbs16(std::string& disasm, uint16_t data);

    void SetFlag(Flags flag, bool set);
    bool GetFlag(Flags flag);

    int TsbDir(); // 0x04
    int PhpImp(); // 0x08
    int OraImm(); // 0x09
    int AslImp(); // 0x0A
    int PhdImp(); // 0x0B
    int BplRel(); // 0x10
    int ClcImp(); // 0x18
    int IncImp(); // 0x1A
    int TcsImp(); // 0x1B
    int TrbAbs(); // 0x1C
    int JsrAbs(); // 0x20
    int JslLng(); // 0x22
    int PlpImp(); // 0x28
    int AndImm(); // 0x29
    int PldImp(); // 0x2B
    int BitAbs(); // 0x2C
    int BmiRel(); // 0x30
    int DecAcc(); // 0x3A
    int AndLnX(); // 0x3F
    int RtiImp(); // 0x40
    int PhaImp(); // 0x48
    int EorImm(); // 0x49
    int LsrImp(); // 0x4A
    int PhkImp(); // 0x4B
    int JmpAbs(); // 0x4C
    int PhyImp(); // 0x5A
    int TcdImp(); // 0x5B
    int JmpLng(); // 0x5C
    int RtsImp(); // 0x60
    int StzDir(); // 0x64
    int AdcDir(); // 0x65
    int AdcDrP(); // 0x67
    int PlaImp(); // 0x68
    int AdcImm(); // 0x69
    int RtlImp(); // 0x6B
    int JmpAbP(); // 0x6C
    int StzDrx(); // 0x74
    int SeiImp(); // 0x78
    int PlyImp(); // 0x7A
    int TdcImp(); // 0x7B
    int BraRel(); // 0x80
    int StyDir(); // 0x84
    int StaDir(); // 0x85
    int StxDir(); // 0x86
    int DeyImp(); // 0x88
    int BitImm(); // 0x89
    int TxaImp(); // 0x8A
    int PhbImp(); // 0x8B
    int StaAbs(); // 0x8D
    int StxAbs(); // 0x8E
    int BccRel(); // 0x90
    int TyaImp(); // 0x98
    int StaAbY(); // 0x99
    int TxsImp(); // 0x9A
    int TxyImp(); // 0x9B
    int StzAbs(); // 0x9C
    int StaAbx(); // 0x9D
    int StzAbx(); // 0x9E
    int LdyImm(); // 0xA0
    int LdxImm(); // 0xA2
    int LdyDir(); // 0xA4
    int LdaDir(); // 0xA5
    int LdaDPL(); // 0xA7 (Direct pointer long (aka [dir]))
    int TayImp(); // 0xA8
    int LdaImm(); // 0xA9
    int TaxImp(); // 0xAA
    int PlbImp(); // 0xAB
    int LdaAbs(); // 0xAD
    int LdxAbs(); // 0xAE
    int LdaDPY(); // 0xA0 [dir],y
    int BcsRel(); // 0xB0
    int LdaDpt(); // 0xB2
    int TyxImp(); // 0xBB
    int LdaAbx(); // 0xBD
    int LdaLnX(); // 0xBF
    int CpyImm(); // 0xC0
    int RepImm(); // 0xC2
    int CpyDir(); // 0xC4
    int CmpDir(); // 0xC5
    int InyImp(); // 0xC8
    int CmpImm(); // 0xC9
    int DexImp(); // 0xCA
    int BneRel(); // 0xD0
    int CldImp(); // 0xD8
    int PhxImp(); // 0xDA
    int CpxImm(); // 0xE0
    int SepImm(); // 0xE2
    int SbcDir(); // 0xE5
    int IncDir(); // 0xE6
    int InxImp(); // 0xE8
    int SbcImm(); // 0xE9
    int XbaImp(); // 0xEB
    int BeqRel(); // 0xF0
    int PeaImm(); // 0xF4
    int PlxImp(); // 0xFA
    int XceImp(); // 0xFB
    int JsrAbx(); // 0xFC
    int SbcLnX(); // 0xFF [long],X

    using CPUFunc = std::function<int()>;
    std::unordered_map<uint8_t, CPUFunc> opcodes;

    bool e = true;

    void Push8(uint8_t data);
    void Push16(uint16_t data);

    uint8_t Pop8();
    uint16_t Pop16();
public:
    CPU();

    void DoNMI();

    int Clock();
    void Dump();
};