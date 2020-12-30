#include "builtin.h"
#include "bytecode.h"
#include "type.h"

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
};

pType PrimitiveTypeBuilder::operator()()const
{
    auto t = typename_backmap.at(s);
    auto r = std::make_shared<PrimitiveType>(t);
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
    r->body = std::static_pointer_cast<ConcreteType>(b);
    return r;
}

using TB = PrimitiveTypeBuilder;

// TODO change some top -> Addressable.
// TODO general support of universal types

std::vector<BuiltinSymbolGenerator::BuiltinFunctionInfo> BuiltinSymbolGenerator::builtin_function_info =
{
    /* Arithmetic */
    { "@addi", {TB("int"), TB("int"), TB("int")}, {{ByteCode::LOADLI, 0, 0}, {ByteCode::LOADLI, 0, 1}, {ByteCode::ADDI}, {ByteCode::RETI} }},
    { "@subi", {TB("int"), TB("int"), TB("int")}, {{ByteCode::LOADLI, 0, 0}, {ByteCode::LOADLI, 0, 1}, {ByteCode::SUBI}, {ByteCode::RETI} }},
    { "@muli", {TB("int"), TB("int"), TB("int")}, {{ByteCode::LOADLI, 0, 0}, {ByteCode::LOADLI, 0, 1}, {ByteCode::MULI}, {ByteCode::RETI} }},
    { "@divi", {TB("int"), TB("int"), TB("int")}, {{ByteCode::LOADLI, 0, 0}, {ByteCode::LOADLI, 0, 1}, {ByteCode::DIVI}, {ByteCode::RETI} }},
    { "@modi", {TB("int"), TB("int"), TB("int")}, {{ByteCode::LOADLI, 0, 0}, {ByteCode::LOADLI, 0, 1}, {ByteCode::REMI}, {ByteCode::RETI} }},
    { "@negi", {TB("int"), TB("int")}, {{ByteCode::LOADLI, 0, 0}, {ByteCode::NEGI}, {ByteCode::RETI} }},
    { "@addf", {TB("float"), TB("float"), TB("float")}, {{ByteCode::LOADLF, 0, 0}, {ByteCode::LOADLF, 0, 1}, {ByteCode::ADDF}, {ByteCode::RETF} }},
    { "@subf", {TB("float"), TB("float"), TB("float")}, {{ByteCode::LOADLF, 0, 0}, {ByteCode::LOADLF, 0, 1}, {ByteCode::SUBF}, {ByteCode::RETF} }},
    { "@mulf", {TB("float"), TB("float"), TB("float")}, {{ByteCode::LOADLF, 0, 0}, {ByteCode::LOADLF, 0, 1}, {ByteCode::MULF}, {ByteCode::RETF} }},
    { "@divf", {TB("float"), TB("float"), TB("float")}, {{ByteCode::LOADLF, 0, 0}, {ByteCode::LOADLF, 0, 1}, {ByteCode::DIVF}, {ByteCode::RETF} }},
    { "@modf", {TB("float"), TB("float"), TB("float")}, {{ByteCode::LOADLF, 0, 0}, {ByteCode::LOADLF, 0, 1}, {ByteCode::REMF}, {ByteCode::RETF} }},
    { "@negf", {TB("float"), TB("float")}, {{ByteCode::LOADLF, 0, 0}, {ByteCode::NEGF}, {ByteCode::RETF} }},

    /* Logic */
    { "@and", {TB("int"), TB("int"), TB("int")}, {{ByteCode::LOADLI, 0, 0}, {ByteCode::LOADLI, 0, 1}, {ByteCode::AND}, {ByteCode::RETI} }},
    { "@or", {TB("int"), TB("int"), TB("int")},  {{ByteCode::LOADLI, 0, 0}, {ByteCode::LOADLI, 0, 1}, {ByteCode::OR}, {ByteCode::RETI} }},
    { "@xor", {TB("int"), TB("int"), TB("int")}, {{ByteCode::LOADLI, 0, 0}, {ByteCode::LOADLI, 0, 1}, {ByteCode::XOR}, {ByteCode::RETI} }},
    { "@not", {TB("int"), TB("int")}, {{ByteCode::LOADLI, 0, 0}, {ByteCode::NOT}, {ByteCode::RETI} }},

    /* Comparison */
    { "@cmpi", {TB("int"), TB("int"), TB("int")}, {{ByteCode::LOADLI, 0, 0}, {ByteCode::LOADLI, 0, 1}, {ByteCode::CMPI}, {ByteCode::RETI} }},
    { "@eqi", {TB("int"), TB("int"), TB("int")},  {{ByteCode::LOADLI, 0, 0}, {ByteCode::LOADLI, 0, 1}, {ByteCode::CMPI}, {ByteCode::EQ}, {ByteCode::RETI} }},
    { "@neqi", {TB("int"), TB("int"), TB("int")}, {{ByteCode::LOADLI, 0, 0}, {ByteCode::LOADLI, 0, 1}, {ByteCode::CMPI}, {ByteCode::NE}, {ByteCode::RETI} }},
    { "@lti", {TB("int"), TB("int"), TB("int")}, {{ByteCode::LOADLI, 0, 0}, {ByteCode::LOADLI, 0, 1}, {ByteCode::CMPI}, {ByteCode::LT}, {ByteCode::RETI} }},
    { "@lei", {TB("int"), TB("int"), TB("int")}, {{ByteCode::LOADLI, 0, 0}, {ByteCode::LOADLI, 0, 1}, {ByteCode::CMPI}, {ByteCode::LE}, {ByteCode::RETI} }},
    { "@gti", {TB("int"), TB("int"), TB("int")}, {{ByteCode::LOADLI, 0, 0}, {ByteCode::LOADLI, 0, 1}, {ByteCode::CMPI}, {ByteCode::GT}, {ByteCode::RETI} }},
    { "@gei", {TB("int"), TB("int"), TB("int")}, {{ByteCode::LOADLI, 0, 0}, {ByteCode::LOADLI, 0, 1}, {ByteCode::CMPI}, {ByteCode::GE}, {ByteCode::RETI} }},
    { "@bool", {TB("int"), TB("bool")}, {{ByteCode::LOADLI, 0, 0}, {ByteCode::CONSTI, 0, 0}, {ByteCode::CMPI}, {ByteCode::NE}, {ByteCode::RET} }},

    { "@cmpf", {TB("float"), TB("float"), TB("int")}, {{ByteCode::LOADLF, 0, 0}, {ByteCode::LOADLF, 0, 1}, {ByteCode::CMPF}, {ByteCode::RETI} }},
    { "@eqf", {TB("float"), TB("float"), TB("int")}, {{ByteCode::LOADLF, 0, 0}, {ByteCode::LOADLF, 0, 1}, {ByteCode::CMPF}, {ByteCode::EQ}, {ByteCode::RETI} }},
    { "@nef", {TB("float"), TB("float"), TB("int")}, {{ByteCode::LOADLF, 0, 0}, {ByteCode::LOADLF, 0, 1}, {ByteCode::CMPF}, {ByteCode::NE}, {ByteCode::RETI} }},
    { "@ltf", {TB("float"), TB("float"), TB("int")}, {{ByteCode::LOADLF, 0, 0}, {ByteCode::LOADLF, 0, 1}, {ByteCode::CMPF}, {ByteCode::LT}, {ByteCode::RETI} }},
    { "@lef", {TB("float"), TB("float"), TB("int")}, {{ByteCode::LOADLF, 0, 0}, {ByteCode::LOADLF, 0, 1}, {ByteCode::CMPF}, {ByteCode::LE}, {ByteCode::RETI} }},
    { "@gtf", {TB("float"), TB("float"), TB("int")}, {{ByteCode::LOADLF, 0, 0}, {ByteCode::LOADLF, 0, 1}, {ByteCode::CMPF}, {ByteCode::GT}, {ByteCode::RETI} }},
    { "@gef", {TB("float"), TB("float"), TB("int")}, {{ByteCode::LOADLF, 0, 0}, {ByteCode::LOADLF, 0, 1}, {ByteCode::CMPF}, {ByteCode::GE}, {ByteCode::RETI} }},

    /* Casting */
    { "@ctoi", {TB("char"), TB("int")},   {{ByteCode::LOADL, 0, 0}, {ByteCode::C2I}, {ByteCode::RETI} }},
    { "@ctof", {TB("char"), TB("float")}, {{ByteCode::LOADL, 0, 0}, {ByteCode::C2F}, {ByteCode::RETF} }},
    { "@itoc", {TB("int"), TB("char")},   {{ByteCode::LOADLI, 0, 0}, {ByteCode::I2C}, {ByteCode::RET} }},
    { "@itof", {TB("int"), TB("float")},  {{ByteCode::LOADLI, 0, 0}, {ByteCode::I2F}, {ByteCode::RETF} }},
    { "@ftoc", {TB("float"), TB("char")}, {{ByteCode::LOADLF, 0, 0}, {ByteCode::F2C}, {ByteCode::RET} }},
    { "@ftoi", {TB("float"), TB("int")},  {{ByteCode::LOADLF, 0, 0}, {ByteCode::F2I}, {ByteCode::RETI} }},

    /* Array operations */
    { "@array",  {TB("int"), TB("array")("char")},  {{ByteCode::LOADLI, 0, 0}, {ByteCode::ALLOC}, {ByteCode::RETA} }},
    { "@arrayi", {TB("int"), TB("array")("int")},   {{ByteCode::LOADLI, 0, 0}, {ByteCode::ALLOCI}, {ByteCode::RETA} }},
    { "@arrayf", {TB("int"), TB("array")("float")}, {{ByteCode::LOADLI, 0, 0}, {ByteCode::ALLOCF}, {ByteCode::RETA} }},
    { "@arraya", {TB("int"), TB("array")("top")}, {{ByteCode::LOADLI, 0, 0}, {ByteCode::ALLOCA}, {ByteCode::RETA} }},

    { "@aget",  {TB("array")("char"), TB("int"), TB("char")},   {{ByteCode::LOADLA, 0, 0}, {ByteCode::LOADLI, 0, 1}, {ByteCode::LOADI}, {ByteCode::RET} }},
    { "@ageti", {TB("array")("int"), TB("int"), TB("int")},     {{ByteCode::LOADLA, 0, 0}, {ByteCode::LOADLI, 0, 1}, {ByteCode::LOADII}, {ByteCode::RETI} }},
    { "@agetf", {TB("array")("float"), TB("int"), TB("float")}, {{ByteCode::LOADLA, 0, 0}, {ByteCode::LOADLI, 0, 1}, {ByteCode::LOADIF}, {ByteCode::RETF} }},
    { "@ageta", {TB("array")("list"), TB("int"), TB("list")},   {{ByteCode::LOADLA, 0, 0}, {ByteCode::LOADLI, 0, 1}, {ByteCode::LOADIA}, {ByteCode::RETA} }},
    { "aget", {TB("array")("list"), TB("int"), TB("list")},   {{ByteCode::LOADLA, 0, 0}, {ByteCode::LOADLI, 0, 1}, {ByteCode::LOADIA}, {ByteCode::RETA} }},

    { "@aset",  {TB("array")("char"), TB("int"), TB("char"), TB("nil")},   {{ByteCode::LOADLA, 0, 0}, {ByteCode::LOADLI, 0, 1}, {ByteCode::LOADL, 0, 2}, {ByteCode::STOREI}, {ByteCode::RETN} }},
    { "@aseti", {TB("array")("int"),  TB("int"), TB("int"), TB("nil")},     {{ByteCode::LOADLA, 0, 0}, {ByteCode::LOADLI, 0, 1}, {ByteCode::LOADLI, 0, 2}, {ByteCode::STOREII}, {ByteCode::RETN} }},
    { "@asetf", {TB("array")("float"),TB("int"), TB("float"), TB("nil")}, {{ByteCode::LOADLA, 0, 0}, {ByteCode::LOADLI, 0, 1}, {ByteCode::LOADLF, 0, 2}, {ByteCode::STOREIF}, {ByteCode::RETN} }},
    { "@aseta", {TB("array")("list"),  TB("int"), TB("list"), TB("nil")}, {{ByteCode::LOADLA, 0, 0}, {ByteCode::LOADLI, 0, 1}, {ByteCode::LOADLA, 0, 2}, {ByteCode::STOREIA}, {ByteCode::RETN} }},
    
    /* Misc */
    { "@print", {TB("array")("char"), TB("nil")}, {{ByteCode::LOADLA, 0, 0}, {ByteCode::CALLNATIVE, 0, 0}, {ByteCode::RETN} }},
    { "@input", {TB("array")("char")}, {{ByteCode::CALLNATIVE, 0, 1}, {ByteCode::RETA} }},
    { "@format", {TB("array")("char"), TB("array")("top"), TB("array")("char")}, {{ByteCode::LOADLA, 0, 0}, {ByteCode::LOADLA, 0, 1}, {ByteCode::CALLNATIVE, 0, 2}, {ByteCode::RETA} }},
    { "@len",   {TB("list"), TB("int")}, {{ByteCode::LOADLA, 0, 0}, {ByteCode::CALLNATIVE, 0, 3}, {ByteCode::RETI} }},
    { "@throw", {TB("top"), TB("bottom")}, {{ByteCode::LOADLA, 0, 0}, {ByteCode::THROW} }},
};