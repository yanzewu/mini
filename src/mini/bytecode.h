#ifndef MINI_BYTECODE_H
#define MINI_BYTECODE_H

#include "stream.h"

#include <stdint.h>
#include <string>
#include <stdexcept>

namespace mini {

    typedef uint32_t Address;
    union StackElem {
        char carg;
        int32_t iarg;
        float farg;
        Address aarg;

        StackElem() : iarg(0) {}
        StackElem(char c) : carg(c) {}
        StackElem(int32_t i) : iarg(i) {}
        StackElem(float f) : farg(f) {}
        StackElem(Address a) : aarg(a) {}
    };

    class ByteCode {
    public:

        enum OpCode : uint16_t {
            NOP = 0x00,
            HALT = 0x01,
            THROW = 0x02,

            LOADL = 0x10,
            LOADLI = 0x11,
            LOADLF = 0x12,
            LOADLA = 0x13,
            LOADI = 0x16,
            LOADII = 0x17,
            LOADIF = 0x18,
            LOADIA = 0x19,
            LOADFIELD = 0x1c,
            LOADINTERFACE = 0x1b,
            LOADG = 0x1d,
            LOADC = 0x1e,

            STOREL = 0x20,
            STORELI = 0x21,
            STORELF = 0x22,
            STORELA = 0x23,
            STOREI = 0x26,
            STOREII = 0x27,
            STOREIF = 0x28,
            STOREIA = 0x29,
            STOREFIELD = 0x2c,
            STOREINTERFACE = 0x2b,
            STOREG = 0x2d,

            ALLOC = 0x30,
            ALLOCI = 0x31,
            ALLOCF = 0x32,
            ALLOCA = 0x33,
            NEW = 0x36,
            NEWCLOSURE = 0x37,

            CALL = 0x40,
            CALLA = 0x41,
            CALLNATIVE = 0x42,
            RETN = 0x43,
            RET = 0x44,
            RETI = 0x45,
            RETF = 0x46,
            RETA = 0x47,

            CONST = 0x50,
            CONSTI = 0x51,
            CONSTF = 0x52,
            CONSTA = 0x53,
            DUP = 0x56,
            POP = 0x58,
            SWAP = 0x5a,
            SHIFT = 0x5c,

            ADDI = 0x60,
            ADDF = 0x61,
            SUBI = 0x64,
            SUBF = 0x65,
            MULI = 0x68,
            MULF = 0x69,
            DIVI = 0x6c,
            DIVF = 0x6d,
            REMI = 0x70,
            REMF = 0x71,
            NEGI = 0x74,
            NEGF = 0x75,
            AND = 0x78,
            OR = 0x7a,
            XOR = 0x7c,
            NOT = 0x7e,

            CMP = 0x80,
            CMPI = 0x81,
            CMPF = 0x82,
            CMPA = 0x83,
            EQ = 0x88,
            NE = 0x89,
            LT = 0x90,
            LE = 0x91,
            GT = 0x92,
            GE = 0x93,

            C2I = 0xa0,
            C2F = 0xa1,
            I2C = 0xa4,
            I2F = 0xa5,
            F2C = 0xa8,
            F2I = 0xa9,
        };

        OpCode code;
        uint16_t arg2;
        StackElem arg1;

        
        // Create a code with no argument
        static ByteCode na_code(ByteCode::OpCode tp, uint16_t type_bit = 0) {
            return { ByteCode::OpCode(tp + type_bit)};
        }

        // Create a code with single argument as address
        static ByteCode sa_code_a(ByteCode::OpCode tp, Address addr, uint16_t type_bit = 0) {
            return { ByteCode::OpCode(tp + type_bit) , 0, StackElem(addr) };
        }

        // Create a code with single argument as address
        static ByteCode sa_code_i(ByteCode::OpCode tp, int32_t index, uint16_t type_bit = 0) {
            return { ByteCode::OpCode(tp + type_bit) , 0, StackElem(index) };
        }

        // Create the code 'consti'
        static ByteCode consti(int32_t i) {
            return { ByteCode::CONSTI, 0, StackElem(i) };
        }

        // Create the code 'consta'
        static ByteCode consta(Address addr) {
            return { ByteCode::CONSTA, 0, StackElem(addr) };
        }

        void print(OutputStream&)const;
    };

    inline OutputStream& operator<<(OutputStream& os, const ByteCode& a) {
        a.print(os); return os;
    }
}

#endif