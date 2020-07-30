#include "ir.h"
#include "stream.h"

#include <unordered_map>
#include <algorithm>

using namespace mini;

const std::unordered_map<ByteCode::OpCode, std::string> code_backmap = {
    {ByteCode::NOP, "nop"},
    {ByteCode::HALT, "halt"},
    {ByteCode::THROW, "throw"},

    {ByteCode::LOADL, "loadlocal"},
    {ByteCode::LOADLI, "loadlocali"},
    {ByteCode::LOADLF, "loadlocalf"},
    {ByteCode::LOADLA, "loadlocala"},
    {ByteCode::LOADI, "loadindex"},
    {ByteCode::LOADII, "loadindexi"},
    {ByteCode::LOADIF, "loadindexf"},
    {ByteCode::LOADIA, "loadindexa"},
    {ByteCode::LOADFIELD, "loadfield"},
    {ByteCode::LOADG, "loadglobal"},
    {ByteCode::LOADC, "loadconst"},

    {ByteCode::STOREL, "storelocal"},
    {ByteCode::STORELI, "storelocali"},
    {ByteCode::STORELF, "storelocalf"},
    {ByteCode::STORELA, "storelocala"},
    {ByteCode::STOREI, "storeindex"},
    {ByteCode::STOREII, "storeindexi"},
    {ByteCode::STOREIF, "storeindexf"},
    {ByteCode::STOREIA, "storeindexa"},
    {ByteCode::STOREFIELD, "storefield"},
    {ByteCode::STOREG, "storeglobal"},

    {ByteCode::ALLOC, "alloc"},
    {ByteCode::ALLOCI, "alloci"},
    {ByteCode::ALLOCF, "allocf"},
    {ByteCode::ALLOCA, "alloca"},
    {ByteCode::NEW, "new"},
    {ByteCode::NEWCLOSURE, "newclosure"},

    {ByteCode::CALL, "call"},
    {ByteCode::CALLA, "calla"},
    {ByteCode::CALLNATIVE, "callnative"},
    {ByteCode::RETN, "retn"},
    {ByteCode::RET, "ret"},
    {ByteCode::RETI, "reti"},
    {ByteCode::RETF, "retf"},
    {ByteCode::RETA, "reta"},

    {ByteCode::CONST, "const"},
    {ByteCode::CONSTI, "consti"},
    {ByteCode::CONSTF, "constf"},
    {ByteCode::CONSTA, "consta"},
    {ByteCode::DUP, "dup"},
    {ByteCode::POP, "pop"},
    {ByteCode::SWAP, "swap"},
    {ByteCode::SHIFT, "shift"},
    
    {ByteCode::ADDI, "addi"},
    {ByteCode::SUBI, "subi"},
    {ByteCode::MULI, "muli"},
    {ByteCode::DIVI, "divi"},
    {ByteCode::REMI, "remi"},
    {ByteCode::NEGI, "negi"},
    {ByteCode::ADDF, "addf"},
    {ByteCode::SUBF, "subf"},
    {ByteCode::MULF, "mulf"},
    {ByteCode::DIVF, "divf"},
    {ByteCode::REMF, "remf"},
    {ByteCode::NEGF, "negf"},

    {ByteCode::AND, "and"},
    {ByteCode::OR, "or"},
    {ByteCode::XOR, "xor"},
    {ByteCode::NOT, "not"},

    {ByteCode::CMP, "cmp"},
    {ByteCode::CMPI, "cmpi"},
    {ByteCode::CMPF, "cmpf"},
    {ByteCode::CMPA, "cmpa"},

    {ByteCode::EQ, "eq"},
    {ByteCode::NE, "ne"},
    {ByteCode::LT, "lt"},
    {ByteCode::LE, "le"},
    {ByteCode::GT, "gt"},
    {ByteCode::GE, "ge"},

    {ByteCode::C2I, "c2i"},
    {ByteCode::C2F, "c2f"},
    {ByteCode::I2C, "i2c"},
    {ByteCode::I2F, "i2f"},
    {ByteCode::F2C, "f2c"},
    {ByteCode::F2I, "f2i"},
};

void appendbuffer_with_indent(OutputStream& os, const std::string& s) {
    os << s;
    os.write_white(20 - s.length());
}

void ByteCode::print(OutputStream& os)const {
    const std::string& s = code_backmap.at(code);
    
    switch (code)
    {
    case ByteCode::LOADL:
    case ByteCode::LOADLI:
    case ByteCode::LOADLF:
    case ByteCode::LOADLA:
    case ByteCode::STOREL:
    case ByteCode::STORELI:
    case ByteCode::STORELF:
    case ByteCode::STORELA:
    case ByteCode::CALLA:
    {
        appendbuffer_with_indent(os, s);
        os << arg1.aarg;
        break;
    }
    case ByteCode::LOADFIELD:
    case ByteCode::LOADG:
    case ByteCode::LOADC:
    case ByteCode::STOREG:
    case ByteCode::STOREFIELD:
    case ByteCode::NEW:
    case ByteCode::NEWCLOSURE:
    case ByteCode::CALL:
    case ByteCode::CALLNATIVE:
    {
        appendbuffer_with_indent(os, s);
        os << '#' << arg1.aarg;
        break;
    }
    case ByteCode::CONST: appendbuffer_with_indent(os, s); os << arg1.carg; break;
    case ByteCode::CONSTI: appendbuffer_with_indent(os, s); os << arg1.iarg; break;
    case ByteCode::CONSTF: appendbuffer_with_indent(os, s); os << arg1.farg; break;
    case ByteCode::CONSTA: appendbuffer_with_indent(os, s); os << arg1.aarg; break;
    default:
        os << s;
        break;
    }
}


OutputStream& Function::print(OutputStream& os, const IRProgram& irprog)const {
    irprog.constant_pool[info_index]->as<FunctionInfo>()->print(os, irprog);
    return print_code(os);
}

OutputStream& Function::print_code(OutputStream& os) const {
    os << "  args_size=" << sz_arg << ", bindings=" << sz_bind << ", locals=" << sz_local << '\n';
    os << "  Code:\n";

    char sbuffer[10];    // ....
    for (unsigned int i = 0; i < codes.size(); i++) {
        sprintf(sbuffer, "%6d: ", i);
        os << (const char*)sbuffer << codes[i] << '\n';
    }
    return os;
}

OutputStream& ClassLayout::print(OutputStream& os, const IRProgram& irprog)const {
    auto info = irprog.constant_pool[info_index]->as<ClassInfo>();
    os << "Class " << irprog.fetch_string(info->name_index) << " [";
    irprog.print_symbolinfo(os, info->symbol_info) <<  "]\n";

    for (size_t i = 0; i < offset.size()-1; i++) {
        os.write_lspace("#" + std::to_string(i), 8) << ": ";
        os.write_lspace("+" + std::to_string(offset[i]), 5);
        os.write_white(2) << irprog.fetch_string(info->field_info[i].name_index) << ':';
        irprog.constant_pool[info->field_info[i].type_index]->as<ClassInfo>()->print_simple(os, irprog) << '\n';
    }
    return os;
}

OutputStream& FunctionInfo::print_simple(OutputStream& os, const IRProgram& irprog)const {
    return irprog.print_string(os, name_index);
}

OutputStream& FunctionInfo::print(OutputStream& os, const IRProgram& irprog)const {
    os << *irprog.constant_pool[name_index]->as<StringConstant>() << ": ";
    os << "function(";
    for (const auto& s : arg_index) {
        irprog.constant_pool[s]->as<ClassInfo>()->print_simple(os, irprog);
        os << ',';
    }
    irprog.constant_pool[ret_index]->as<ClassInfo>()->print_simple(os, irprog);
    os << ')';
    os << " [";
    irprog.print_symbolinfo(os, symbol_info);
    os << ']';
    return os;
}

OutputStream& ClassInfo::print_simple(OutputStream& os, const IRProgram& irprog)const {
    return irprog.print_string(os, name_index);
}

const LineNumberTable::LineNumberPair& LineNumberTable::query(Size_t function_addr, Size_t pc) const {

    auto r = std::lower_bound(line_number_table.begin(), line_number_table.end(), LineNumberPair{ function_addr, pc });
    if (r == line_number_table.end()) {
        if (line_number_table.back().function_index == function_addr) {  // happen to be the last one, without multiple lines
            return line_number_table.back();
        }
        else {
            throw std::runtime_error("Corresponding line number does not exist");
        }
    }
    else {
        return *r;
    }
}

OutputStream& LineNumberTable::print_line_number(OutputStream& os, Size_t function_addr) const {

    os << "  LineNumberTable:\n";
    for (size_t j = &query(function_addr, 0) - &line_number_table[0];
        j < line_number_table.size() && line_number_table[j].function_index == function_addr;
        j++) {
        os << "    line " << line_number_table[j].line_number + 1 << ": " << line_number_table[j].pc << '\n';
    }
    return os;
}

OutputStream& IRProgram::print(OutputStream& os, bool with_head, bool with_lnt)const {
    if (with_head) {
        os << "Compiled from \"" << constant_pool[source_index]->as<StringConstant>()->value << "\"\n";
    }

    for (size_t i = 0; i < constant_pool.size(); i++) {
        const auto& cp = constant_pool[i];
        if (cp->get_type() == ConstantPoolObject::FUNCTION) {
            cp->as<Function>()->print(os, *this) << '\n';
            if (with_lnt) {
                line_number_table()->print_line_number(os, i);
            }
            os << '\n';
        }
        else if (cp->get_type() == ConstantPoolObject::CLASS_LAYOUT) {
            cp->as<ClassLayout>()->print(os, *this) << '\n';
        }
    }
    return os;
}

OutputStream& IRProgram::print_symbolinfo(OutputStream& os, const SymbolInfo& info) const {
    if (info.is_absolute()) {
        return os << info;
    }
    else {
        return os << fetch_string(info.location.srcno) << ':' << info.location.lineno + 1;
    }
}

OutputStream& IRProgram::print_full(OutputStream& os)const {
    os << "Compiled from \"";
    print_string(os, source_index);
    os << "\"\n";
    os << "  Entry:         #" << entry_index << '\n';
    os << "  Global Region: #" << global_pool_index << '\n';
    os << "Constant Pool:\n";
    for (size_t i = 0; i < constant_pool.size(); i++) {
        os.write_lspace("#" + std::to_string(i), 6) << " = ";
        switch (constant_pool[i]->get_type())
        {
        case ConstantPoolObject::STRING: os.write_rspace("String", 20) << *constant_pool[i]; break;
        case ConstantPoolObject::CLASS_LAYOUT: os.write_rspace("Class", 20) << *constant_pool[i];
            os.write_white(10) << "; ";
            constant_pool[constant_pool[i]->as<ClassLayout>()->info_index]->as<ClassInfo>()->print_simple(os, *this);
            break;
        case ConstantPoolObject::FUNCTION: os.write_rspace("Function", 20) << *constant_pool[i];
            os.write_white(10) << "; ";
            constant_pool[constant_pool[i]->as<Function>()->info_index]->as<FunctionInfo>()->print_simple(os, *this);
            break;
        case ConstantPoolObject::CLASS_INFO: os.write_rspace("ClassInfo", 20) << *constant_pool[i]; break;
        case ConstantPoolObject::FUNCTION_INFO: os.write_rspace("ClassInfo", 20) << *constant_pool[i]; break;
        case ConstantPoolObject::LINE_NUMBER_TABLE: os.write_rspace("LineNumberTable", 20) << *constant_pool[i]; break;
        default:
            break;
        }
        os << '\n';
    }
    os << '\n';
    print(os, false, true);
    return os;
}


