#include "builtin.h"
#include "bytecode.h"
#include "type.h"
#include "native.h"

using namespace mini;


std::vector<BuiltinSymbolGenerator::BuiltinTypeInfo> BuiltinSymbolGenerator::builtin_type_info =
{
    {PrimitiveTypeMetaData::TOP, "top"},
    {PrimitiveTypeMetaData::NIL, "nil"},
    {PrimitiveTypeMetaData::BOOL, "bool"},
    {PrimitiveTypeMetaData::CHAR, "char"},
    {PrimitiveTypeMetaData::INT, "int"},
    {PrimitiveTypeMetaData::FLOAT, "float"},
    {PrimitiveTypeMetaData::LIST, "list"},
    {PrimitiveTypeMetaData::ARRAY, "array"},
    {PrimitiveTypeMetaData::TUPLE, "tuple"},
    {PrimitiveTypeMetaData::FUNCTION, "function"},
    {PrimitiveTypeMetaData::OBJECT, "object"},
    {PrimitiveTypeMetaData::BOTTOM, "bottom"},
    {PrimitiveTypeMetaData::ADDRESSABLE, "@Addressable"},
    {PrimitiveTypeMetaData::VARIANT, "variant"},
};

std::unordered_map<std::string, ConstTypedefRef> typename_backmap =
{
    {"top",     new PrimitiveTypeMetaData(Symbol::create_absolute_symbol("top"), PrimitiveTypeMetaData::TOP)},
    {"nil",     new PrimitiveTypeMetaData(Symbol::create_absolute_symbol("nil"), PrimitiveTypeMetaData::NIL)},
    {"bool",     new PrimitiveTypeMetaData(Symbol::create_absolute_symbol("bool"), PrimitiveTypeMetaData::BOOL)},
    {"char",    new PrimitiveTypeMetaData(Symbol::create_absolute_symbol("char"), PrimitiveTypeMetaData::CHAR)},
    {"int",     new PrimitiveTypeMetaData(Symbol::create_absolute_symbol("int"), PrimitiveTypeMetaData::INT)},
    {"float",   new PrimitiveTypeMetaData(Symbol::create_absolute_symbol("float"), PrimitiveTypeMetaData::FLOAT)},
    {"list",    new PrimitiveTypeMetaData(Symbol::create_absolute_symbol("list"), PrimitiveTypeMetaData::LIST)},
    {"array",   new PrimitiveTypeMetaData(Symbol::create_absolute_symbol("array"), PrimitiveTypeMetaData::ARRAY)},
    {"tuple",   new PrimitiveTypeMetaData(Symbol::create_absolute_symbol("tuple"), PrimitiveTypeMetaData::TUPLE)},
    {"function", new PrimitiveTypeMetaData(Symbol::create_absolute_symbol("function"), PrimitiveTypeMetaData::FUNCTION)},
    {"object", new PrimitiveTypeMetaData(Symbol::create_absolute_symbol("object"), PrimitiveTypeMetaData::OBJECT)},
    {"bottom",  new PrimitiveTypeMetaData(Symbol::create_absolute_symbol("bottom"), PrimitiveTypeMetaData::BOTTOM)},
    {"@Addressable",  new PrimitiveTypeMetaData(Symbol::create_absolute_symbol("@Addressable"), PrimitiveTypeMetaData::ADDRESSABLE)},
};

std::unordered_map<std::string, PrimitiveTypeBuilder*> typebuilder_backmap =
{
    {"top", new PrimitiveTypeBuilder("top")},
    {"nil", new PrimitiveTypeBuilder("nil")},
    {"char", new PrimitiveTypeBuilder("char")},
    {"int", new PrimitiveTypeBuilder("int")},
    {"float", new PrimitiveTypeBuilder("float")},
    {"list", new PrimitiveTypeBuilder("list")},
    {"object", new PrimitiveTypeBuilder("object")},
    {"bottom", new PrimitiveTypeBuilder("bottom")},
    {"@Addressable", new PrimitiveTypeBuilder("@Addressable")},
};

pType PrimitiveTypeBuilder::operator()()const
{
    auto t = typename_backmap.at(s);
    auto r = std::make_shared<PrimitiveType>(t->as<PrimitiveTypeMetaData>());
    for (const auto& a : args) {
        r->args.push_back((*a)());
    }
    return r;
}

PrimitiveTypeBuilder& PrimitiveTypeBuilder::operator()(const std::string& s) {
    args.push_back(typebuilder_backmap[s]);
    return *this;
}

pType PrimitiveTypeBuilder::operator()(const SymbolTable& st) const
{
    auto t = st.find_type(s);
    auto r = std::make_shared<PrimitiveType>(t);
    for (const auto& a : args) {
        r->args.push_back((*a)(st));
    }
    return r;
}

pType PrimitiveTypeBuilder::build(SymbolTable& st) const
{
    auto t = st.find_type(s);
    auto r = std::make_shared<PrimitiveType>(t);
    for (const auto& a : args) {
        r->args.push_back(a->build(st));
    }
    return r;
}

pType StructTypeBuilder::operator()()const {
    auto r = std::make_shared<StructType>();
    for (const auto& f : fields) {
        r->fields[f.first] = (*f.second)();
    }
    return r;
}

pType StructTypeBuilder::operator()(const SymbolTable& st) const
{
    auto r = std::make_shared<StructType>();
    for (const auto& f : fields) {
        r->fields[f.first] = (*f.second)(st);
    }
    return r;
}

pType StructTypeBuilder::build(SymbolTable& st) const
{
    auto r = std::make_shared<StructType>();
    for (const auto& f : fields) {
        r->fields[f.first] = (*f.second)(st);
    }
    return r;
}

pType TypeVariableBuilder::build(SymbolTable& st) const
{
    std::pair<ConstTypedefRef, unsigned> p = st.find_type(s, SymbolInfo(Location(0,0,0)));
    if (!p.first || !p.first->is_variable()) throw std::runtime_error("Not a variable");
    return std::make_shared<UniversalTypeVariable>(p.second, p.first->as<TypeVariableMetaData>()->arg_id,
        p.first->as<TypeVariableMetaData>()->quantifier.get());
}


pType UniversalTypeBuilder::build(SymbolTable& st)const
{
    auto r = std::make_shared<UniversalType>();
    st.push_type_scope();
    for (size_t i = 0; i < quantifiers.size(); i++) {
        if (quantifiers[i].second) {
            r->quantifiers.push_back((*quantifiers[i].second)());
            st.insert_type_var(std::make_shared<Symbol>(quantifiers[i].first, SymbolInfo(Location(0,0,0))), i, r->quantifiers.back());
        }
        else {
            r->quantifiers.push_back(std::make_shared<PrimitiveType>(st.find_type("@Addressable")));
            st.insert_type_var(std::make_shared<Symbol>(quantifiers[i].first, SymbolInfo(Location(0, 0, 0))), i, r->quantifiers.back());
        }
    }
    auto b = body->build(st);
    st.pop_type_scope();

    if (!b->is_concrete()) throw std::runtime_error("Need a concrete type");
    r->body = b;
    return r;
}

using TB = PrimitiveTypeBuilder;
using UTB = UniversalTypeBuilder;
using VTB = TypeVariableBuilder;

const TypeBuilder* f_int_int_int = &(*new TB("function"))("int")("int")("int");
const TypeBuilder* f_float_float_float = &(*new TB("function"))("float")("float")("float");
const TypeBuilder* f_float_float_int = &(*new TB("function"))("float")("float")("int");
const TypeBuilder* array_char = &(*new TB("array"))("char");
const TypeBuilder* array_int = &(*new TB("array"))("int");
const TypeBuilder* array_float = &(*new TB("array"))("float");
const TypeBuilder* array_addressable = &(*new TB("array"))("@Addressable");
const TypeBuilder* array_x = &(*new TB("array"))(*new VTB("x"));

std::vector<BuiltinSymbolGenerator::BuiltinFunctionInfo> BuiltinSymbolGenerator::builtin_function_info =
{
    /* Arithmetic */
    { "@addi", f_int_int_int, {{ByteCode::LOADLI, 0, 0}, {ByteCode::LOADLI, 0, 1}, {ByteCode::ADDI}, {ByteCode::RETI} }},
    { "@subi", f_int_int_int, {{ByteCode::LOADLI, 0, 0}, {ByteCode::LOADLI, 0, 1}, {ByteCode::SUBI}, {ByteCode::RETI} }},
    { "@muli", f_int_int_int, {{ByteCode::LOADLI, 0, 0}, {ByteCode::LOADLI, 0, 1}, {ByteCode::MULI}, {ByteCode::RETI} }},
    { "@divi", f_int_int_int, {{ByteCode::LOADLI, 0, 0}, {ByteCode::LOADLI, 0, 1}, {ByteCode::DIVI}, {ByteCode::RETI} }},
    { "@modi", f_int_int_int, {{ByteCode::LOADLI, 0, 0}, {ByteCode::LOADLI, 0, 1}, {ByteCode::REMI}, {ByteCode::RETI} }},
    { "@negi", &(*new TB("function"))("int")("int"), {{ByteCode::LOADLI, 0, 0}, {ByteCode::NEGI}, {ByteCode::RETI} }},
    { "@addf", f_float_float_float, {{ByteCode::LOADLF, 0, 0}, {ByteCode::LOADLF, 0, 1}, {ByteCode::ADDF}, {ByteCode::RETF} }},
    { "@subf", f_float_float_float, {{ByteCode::LOADLF, 0, 0}, {ByteCode::LOADLF, 0, 1}, {ByteCode::SUBF}, {ByteCode::RETF} }},
    { "@mulf", f_float_float_float, {{ByteCode::LOADLF, 0, 0}, {ByteCode::LOADLF, 0, 1}, {ByteCode::MULF}, {ByteCode::RETF} }},
    { "@divf", f_float_float_float, {{ByteCode::LOADLF, 0, 0}, {ByteCode::LOADLF, 0, 1}, {ByteCode::DIVF}, {ByteCode::RETF} }},
    { "@modf", f_float_float_float, {{ByteCode::LOADLF, 0, 0}, {ByteCode::LOADLF, 0, 1}, {ByteCode::REMF}, {ByteCode::RETF} }},
    { "@negf",  &(*new TB("function"))("float")("float"), {{ByteCode::LOADLF, 0, 0}, {ByteCode::NEGF}, {ByteCode::RETF} }},

    /* Logic */
    { "@and", f_int_int_int, {{ByteCode::LOADLI, 0, 0}, {ByteCode::LOADLI, 0, 1}, {ByteCode::AND}, {ByteCode::RETI} }},
    { "@or", f_int_int_int,  {{ByteCode::LOADLI, 0, 0}, {ByteCode::LOADLI, 0, 1}, {ByteCode::OR}, {ByteCode::RETI} }},
    { "@xor", f_int_int_int, {{ByteCode::LOADLI, 0, 0}, {ByteCode::LOADLI, 0, 1}, {ByteCode::XOR}, {ByteCode::RETI} }},
    { "@not", &(*new TB("function"))("int")("int"), {{ByteCode::LOADLI, 0, 0}, {ByteCode::NOT}, {ByteCode::RETI} }},

    /* Comparison */
    { "@cmpi", f_int_int_int, {{ByteCode::LOADLI, 0, 0}, {ByteCode::LOADLI, 0, 1}, {ByteCode::CMPI}, {ByteCode::RETI} }},
    { "@eqi", f_int_int_int,  {{ByteCode::LOADLI, 0, 0}, {ByteCode::LOADLI, 0, 1}, {ByteCode::CMPI}, {ByteCode::EQ}, {ByteCode::RETI} }},
    { "@nei", f_int_int_int, {{ByteCode::LOADLI, 0, 0}, {ByteCode::LOADLI, 0, 1}, {ByteCode::CMPI}, {ByteCode::NE}, {ByteCode::RETI} }},
    { "@lti", f_int_int_int, {{ByteCode::LOADLI, 0, 0}, {ByteCode::LOADLI, 0, 1}, {ByteCode::CMPI}, {ByteCode::LT}, {ByteCode::RETI} }},
    { "@lei", f_int_int_int, {{ByteCode::LOADLI, 0, 0}, {ByteCode::LOADLI, 0, 1}, {ByteCode::CMPI}, {ByteCode::LE}, {ByteCode::RETI} }},
    { "@gti", f_int_int_int, {{ByteCode::LOADLI, 0, 0}, {ByteCode::LOADLI, 0, 1}, {ByteCode::CMPI}, {ByteCode::GT}, {ByteCode::RETI} }},
    { "@gei", f_int_int_int, {{ByteCode::LOADLI, 0, 0}, {ByteCode::LOADLI, 0, 1}, {ByteCode::CMPI}, {ByteCode::GE}, {ByteCode::RETI} }},
    { "@bool", &(*new TB("function"))("top")("int"), {{ByteCode::LOADLI, 0, 0}, {ByteCode::CONSTI, 0, 0}, {ByteCode::CMPI}, {ByteCode::NE}, {ByteCode::RETI} }},

    { "@cmpf", f_float_float_int, {{ByteCode::LOADLF, 0, 0}, {ByteCode::LOADLF, 0, 1}, {ByteCode::CMPF}, {ByteCode::RETI} }},
    { "@eqf", f_float_float_int, {{ByteCode::LOADLF, 0, 0}, {ByteCode::LOADLF, 0, 1}, {ByteCode::CMPF}, {ByteCode::EQ}, {ByteCode::RETI} }},
    { "@nef", f_float_float_int, {{ByteCode::LOADLF, 0, 0}, {ByteCode::LOADLF, 0, 1}, {ByteCode::CMPF}, {ByteCode::NE}, {ByteCode::RETI} }},
    { "@ltf", f_float_float_int, {{ByteCode::LOADLF, 0, 0}, {ByteCode::LOADLF, 0, 1}, {ByteCode::CMPF}, {ByteCode::LT}, {ByteCode::RETI} }},
    { "@lef", f_float_float_int, {{ByteCode::LOADLF, 0, 0}, {ByteCode::LOADLF, 0, 1}, {ByteCode::CMPF}, {ByteCode::LE}, {ByteCode::RETI} }},
    { "@gtf", f_float_float_int, {{ByteCode::LOADLF, 0, 0}, {ByteCode::LOADLF, 0, 1}, {ByteCode::CMPF}, {ByteCode::GT}, {ByteCode::RETI} }},
    { "@gef", f_float_float_int, {{ByteCode::LOADLF, 0, 0}, {ByteCode::LOADLF, 0, 1}, {ByteCode::CMPF}, {ByteCode::GE}, {ByteCode::RETI} }},

    /* Casting */
    { "@ctoi", &(*new TB("function"))("char")("int"),   {{ByteCode::LOADL, 0, 0}, {ByteCode::C2I}, {ByteCode::RETI} }},
    { "@ctof", &(*new TB("function"))("char")("float"), {{ByteCode::LOADL, 0, 0}, {ByteCode::C2F}, {ByteCode::RETF} }},
    { "@itoc", &(*new TB("function"))("int")("char"),   {{ByteCode::LOADLI, 0, 0}, {ByteCode::I2C}, {ByteCode::RET} }},
    { "@itof", &(*new TB("function"))("int")("float"),  {{ByteCode::LOADLI, 0, 0}, {ByteCode::I2F}, {ByteCode::RETF} }},
    { "@ftoc", &(*new TB("function"))("float")("char"), {{ByteCode::LOADLF, 0, 0}, {ByteCode::F2C}, {ByteCode::RET} }},
    { "@ftoi", &(*new TB("function"))("float")("int"),  {{ByteCode::LOADLF, 0, 0}, {ByteCode::F2I}, {ByteCode::RETI} }},

    /* Array operations */
    { "@array",  &(*new TB("function"))("int")(*array_char),  {{ByteCode::LOADLI, 0, 0}, {ByteCode::ALLOC}, {ByteCode::RETA} }},
    { "@arrayi", &(*new TB("function"))("int")(*array_int),   {{ByteCode::LOADLI, 0, 0}, {ByteCode::ALLOCI}, {ByteCode::RETA} }},
    { "@arrayf", &(*new TB("function"))("int")(*array_float), {{ByteCode::LOADLI, 0, 0}, {ByteCode::ALLOCF}, {ByteCode::RETA} }},
    
    { "@aget",  &(*new TB("function"))(*array_char)("int")("char"),   {{ByteCode::LOADLA, 0, 0}, {ByteCode::LOADLI, 0, 1}, {ByteCode::LOADI}, {ByteCode::RET} }},
    { "@ageti", &(*new TB("function"))(*array_int)("int")("int"),     {{ByteCode::LOADLA, 0, 0}, {ByteCode::LOADLI, 0, 1}, {ByteCode::LOADII}, {ByteCode::RETI} }},
    { "@agetf", &(*new TB("function"))(*array_float)("int")("float"), {{ByteCode::LOADLA, 0, 0}, {ByteCode::LOADLI, 0, 1}, {ByteCode::LOADIF}, {ByteCode::RETF} }},

    { "@aset",  &(*new TB("function"))(*array_char)("int")("char")("nil"),   {{ByteCode::LOADLA, 0, 0}, {ByteCode::LOADLI, 0, 1}, {ByteCode::LOADL, 0, 2}, {ByteCode::STOREI}, {ByteCode::RETN} }},
    { "@aseti", &(*new TB("function"))(*array_char)("int")("int")("nil"),     {{ByteCode::LOADLA, 0, 0}, {ByteCode::LOADLI, 0, 1}, {ByteCode::LOADLI, 0, 2}, {ByteCode::STOREII}, {ByteCode::RETN} }},
    { "@asetf", &(*new TB("function"))(*array_char)("int")("float")("nil"), {{ByteCode::LOADLA, 0, 0}, {ByteCode::LOADLI, 0, 1}, {ByteCode::LOADLF, 0, 2}, {ByteCode::STOREIF}, {ByteCode::RETN} }},
    
    /* Memory */
    { "@alloc", &(*new UTB())("x")( (*new TB("function"))("int")(*array_x) ),
        {{ByteCode::LOADLI, 0, 0}, {ByteCode::ALLOCA}, {ByteCode::RETA} }},
    { "@get",  &(*new UTB())("x")( (*new TB("function"))("@Addressable")("int")(*new VTB("x")) ),
        {{ByteCode::LOADLA, 0, 0}, {ByteCode::LOADLI, 0, 1}, {ByteCode::LOADIA}, {ByteCode::RETA} }},
    { "@set",  &(*new UTB())("x")( (*new TB("function"))(*array_x)("int")(*new VTB("x"))("nil") ),
        {{ByteCode::LOADLA, 0, 0}, {ByteCode::LOADLI, 0, 1}, {ByteCode::LOADLA, 0, 2}, {ByteCode::STOREIA}, {ByteCode::RETN} }},

    /* Error */
    { "@throw", &(*new TB("function"))("top")("bottom"), {{ByteCode::LOADLA, 0, 0}, {ByteCode::THROW} }},

    /* Native */
    { "@len",   &(*new TB("function"))("@Addressable")("int"),
        {{ByteCode::LOADLA, 0, 0}, {ByteCode::CALLNATIVE, 0, (int)NativeFunction::LEN}, {ByteCode::RETI} }},
    { "@copy",  &(*new UTB())("x")( (*new TB("function"))(*array_x)("int")("int")(*array_x)("int")("nil") ),
        {{ByteCode::LOADLA, 0, 0}, {ByteCode::LOADLI, 0, 1}, {ByteCode::LOADLI, 0, 2}, {ByteCode::LOADLA, 0, 3}, {ByteCode::LOADLI, 0, 4}, {ByteCode::CALLNATIVE, 0, (int)NativeFunction::COPY}, {ByteCode::RETN}}},
    { "@open",  &(*new TB("function"))(*array_char)(*array_char)("int"),
        {{ByteCode::LOADLA, 0, 0}, {ByteCode::LOADLA, 0, 1}, {ByteCode::CALLNATIVE, 0, (int)NativeFunction::OPEN}, {ByteCode::RETI} }},
    { "@close", &(*new TB("function"))("int")("nil"),
        {{ByteCode::LOADLI, 0, 0}, {ByteCode::CALLNATIVE, 0, (int)NativeFunction::CLOSE}, {ByteCode::RETN} }},
    { "@read",  &(*new TB("function"))("int")("int")("char")(*array_char),
        {{ByteCode::LOADLI, 0, 0}, {ByteCode::LOADLI, 0, 1}, {ByteCode::LOADL, 0, 2}, {ByteCode::CALLNATIVE, 0, (int)NativeFunction::READ}, {ByteCode::RETA} }},
    { "@write", &(*new TB("function"))("int")(*array_char)("nil"),
        {{ByteCode::LOADLI, 0, 0}, {ByteCode::LOADLA, 0, 1}, {ByteCode::CALLNATIVE, 0, (int)NativeFunction::WRITE}, {ByteCode::RETN} }},
    { "@format", &(*new TB("function"))(*array_char)(*array_char)(*array_char),
        {{ByteCode::LOADLA, 0, 0}, {ByteCode::LOADLA, 0, 1}, {ByteCode::CALLNATIVE, 0, (int)NativeFunction::FORMAT}, {ByteCode::RETA} }},
    
    
};