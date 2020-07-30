#include "builtin.h"
#include "bytecode.h"
#include "type.h"

using namespace mini;



// for typebuilder

std::unordered_map<std::string, ConstTypedefRef> typename_backmap =
{
    {"object",  new TypeMetaData(Symbol::create_absolute_symbol("object"), TypeMetaData::OBJECT)},
    {"nil",     new TypeMetaData(Symbol::create_absolute_symbol("nil"), TypeMetaData::NIL)},
    {"char",    new TypeMetaData(Symbol::create_absolute_symbol("char"), TypeMetaData::CHAR)},
    {"int",     new TypeMetaData(Symbol::create_absolute_symbol("int"), TypeMetaData::INT)},
    {"float",   new TypeMetaData(Symbol::create_absolute_symbol("float"), TypeMetaData::FLOAT)},
    {"list",    new TypeMetaData(Symbol::create_absolute_symbol("list"), TypeMetaData::LIST)},
    {"array",   new TypeMetaData(Symbol::create_absolute_symbol("array"), TypeMetaData::ARRAY)},
    {"tuple",   new TypeMetaData(Symbol::create_absolute_symbol("tuple"), TypeMetaData::TUPLE)},
    {"struct",  new TypeMetaData(Symbol::create_absolute_symbol("struct"), TypeMetaData::STRUCT)},
    {"function", new TypeMetaData(Symbol::create_absolute_symbol("function"), TypeMetaData::FUNCTION)},
    {"bottom",  new TypeMetaData(Symbol::create_absolute_symbol("bottom"), TypeMetaData::BOTTOM)},
};

pType BuiltinTypeBuilder::operator()()const
{
    auto t = typename_backmap.at(s);
    if (t->get_type() == TypeMetaData::STRUCT) {
        auto s = std::make_shared<StructType>(t);
        for (const auto& f : fields) {
            s->fields[f.first] = f.second();
        }
        return s;
    }
    else {
        auto s = std::make_shared<Type>(t);
        for (const auto& a : args) {
            s->args.push_back(a());
        }
        return s;
    }
}

pType BuiltinTypeBuilder::operator()(const SymbolTable& st) const
{
    auto t = st.find_type(s);
    if (t->get_type() == TypeMetaData::STRUCT) {
        auto s = std::make_shared<StructType>(t);
        for (const auto& f : fields) {
            s->fields[f.first] = f.second(st);
        }
        return s;
    }
    else {
        auto s = std::make_shared<Type>(t);
        for (const auto& a : args) {
            s->args.push_back(a(st));
        }
        return s;
    }
}


std::vector<BuiltinSymbolGenerator::BuiltinTypeInfo> BuiltinSymbolGenerator::builtin_type_info =
{
    {TypeMetaData::OBJECT, "object", 0, 0},
    {TypeMetaData::NIL, "nil", 0, 0},
    {TypeMetaData::BOOL, "bool", 0, 0},
    {TypeMetaData::INT, "int", 0, 0},
    {TypeMetaData::CHAR, "char", 0, 0},
    {TypeMetaData::FLOAT, "float", 0, 0},
    {TypeMetaData::LIST, "list", 0, 0},
    {TypeMetaData::BOTTOM, "bottom", 0, 0},
    {TypeMetaData::FUNCTION, "function", 1, UINT_MAX},
    {TypeMetaData::TUPLE, "tuple", 1, UINT_MAX},
    {TypeMetaData::ARRAY, "array", 1, 1},
    {TypeMetaData::STRUCT, "struct", 1, UINT_MAX}
};

using TB = BuiltinTypeBuilder;

std::vector<BuiltinSymbolGenerator::BuiltinFunctionInfo> BuiltinSymbolGenerator::builtin_function_info =
{
    { "@addi", {TB("int"), TB("int"), TB("int")}, {
        {ByteCode::LOADLI, 0, 0}, {ByteCode::LOADLI, 0, 1}, {ByteCode::ADDI}, {ByteCode::RETI} }},
    { "@subi", {TB("int"), TB("int"), TB("int")}, {
        {ByteCode::LOADLI, 0, 0}, {ByteCode::LOADLI, 0, 1}, {ByteCode::SUBI}, {ByteCode::RETI} }},
    { "@muli", {TB("int"), TB("int"), TB("int")}, {
        {ByteCode::LOADLI, 0, 0}, {ByteCode::LOADLI, 0, 1}, {ByteCode::MULI}, {ByteCode::RETI} }},
    { "@divi", {TB("int"), TB("int"), TB("int")}, {
        {ByteCode::LOADLI, 0, 0}, {ByteCode::LOADLI, 0, 1}, {ByteCode::DIVI}, {ByteCode::RETI} }},
    { "@modi", {TB("int"), TB("int"), TB("int")}, {
        {ByteCode::LOADLI, 0, 0}, {ByteCode::LOADLI, 0, 1}, {ByteCode::REMI}, {ByteCode::RETI} }},
    { "@negi", {TB("int"), TB("int")}, {
        {ByteCode::LOADLI, 0, 0}, {ByteCode::NEGI}, {ByteCode::RETI} }},
    { "@andi", {TB("int"), TB("int"), TB("int")}, {
        {ByteCode::LOADLI, 0, 0}, {ByteCode::LOADLI, 0, 1}, {ByteCode::AND}, {ByteCode::RETI} }},
    { "@ori", {TB("int"), TB("int"), TB("int")}, {
        {ByteCode::LOADLI, 0, 0}, {ByteCode::LOADLI, 0, 1}, {ByteCode::OR}, {ByteCode::RETI} }},
    { "@xori", {TB("int"), TB("int"), TB("int")}, {
        {ByteCode::LOADLI, 0, 0}, {ByteCode::LOADLI, 0, 1}, {ByteCode::XOR}, {ByteCode::RETI} }},
    { "@noti", {TB("int"), TB("int")}, {
        {ByteCode::LOADLI, 0, 0}, {ByteCode::NOT}, {ByteCode::RETI} }},
    { "@cmpi", {TB("int"), TB("int"), TB("int")}, {
        {ByteCode::LOADLI, 0, 0}, {ByteCode::LOADLI, 0, 1}, {ByteCode::CMPI}, {ByteCode::RETI} }},
    { "@eqi", {TB("int"), TB("int"), TB("int")}, {
        {ByteCode::LOADLI, 0, 0}, {ByteCode::LOADLI, 0, 1}, {ByteCode::CMPI}, {ByteCode::EQ}, {ByteCode::RETI} }},
    { "@neqi", {TB("int"), TB("int"), TB("int")}, {
        {ByteCode::LOADLI, 0, 0}, {ByteCode::LOADLI, 0, 1}, {ByteCode::CMPI}, {ByteCode::NE}, {ByteCode::RETI} }},
    { "@lessi", {TB("int"), TB("int"), TB("int")}, {
        {ByteCode::LOADLI, 0, 0}, {ByteCode::LOADLI, 0, 1}, {ByteCode::CMPI}, {ByteCode::LT}, {ByteCode::RETI} }},
    { "@lesseqi", {TB("int"), TB("int"), TB("int")}, {
        {ByteCode::LOADLI, 0, 0}, {ByteCode::LOADLI, 0, 1}, {ByteCode::CMPI}, {ByteCode::LE}, {ByteCode::RETI} }},
    { "@greateri", {TB("int"), TB("int"), TB("int")}, {
        {ByteCode::LOADLI, 0, 0}, {ByteCode::LOADLI, 0, 1}, {ByteCode::CMPI}, {ByteCode::GT}, {ByteCode::RETI} }},
    { "@greatereqi", {TB("int"), TB("int"), TB("int")}, {
        {ByteCode::LOADLI, 0, 0}, {ByteCode::LOADLI, 0, 1}, {ByteCode::CMPI}, {ByteCode::GE}, {ByteCode::RETI} }},
    { "@eqf", {TB("float"), TB("float"), TB("int")}, {
        {ByteCode::LOADLF, 0, 0}, {ByteCode::LOADLF, 0, 1}, {ByteCode::CMPF}, {ByteCode::EQ}, {ByteCode::RETI} }},
    { "@ctoi", {TB("char"), TB("int")}, {
        {ByteCode::LOADL, 0, 0}, {ByteCode::C2I}, {ByteCode::RETI} }},
    { "@ctof", {TB("char"), TB("float")}, {
        {ByteCode::LOADL, 0, 0}, {ByteCode::C2F}, {ByteCode::RETF} }},
    { "@itoc", {TB("int"), TB("char")}, {
        {ByteCode::LOADLI, 0, 0}, {ByteCode::I2C}, {ByteCode::RET} }},
    { "@itof", {TB("int"), TB("float")}, {
        {ByteCode::LOADLI, 0, 0}, {ByteCode::I2F}, {ByteCode::RETF} }},
    { "@ftoc", {TB("float"), TB("char")}, {
        {ByteCode::LOADLF, 0, 0}, {ByteCode::F2C}, {ByteCode::RET} }},
    { "@ftoi", {TB("float"), TB("int")}, {
        {ByteCode::LOADLF, 0, 0}, {ByteCode::F2I}, {ByteCode::RETI} }},
    { "@arrayi", {TB("int"), TB("array")("int")}, {
        {ByteCode::LOADLI, 0, 0}, {ByteCode::ALLOCI}, {ByteCode::RETA} }},
    { "@indexi", {TB("array")("int"), TB("int"), TB("int")}, {
        {ByteCode::LOADLA, 0, 0}, {ByteCode::LOADLI, 0, 1}, {ByteCode::LOADII}, {ByteCode::RETI} }},
    { "@setindexi", {TB("array")("int"), TB("int"), TB("int"), TB("nil")}, {
        {ByteCode::LOADLA, 0, 0}, {ByteCode::LOADLI, 0, 1}, {ByteCode::LOADLI, 0, 2}, {ByteCode::STOREII}, {ByteCode::RETN} }},
    { "@print", {TB("array")("char"), TB("nil")}, {
        {ByteCode::LOADLA, 0, 0}, {ByteCode::CALLNATIVE, 0, 0}, {ByteCode::RETN} }},
    { "@input", {TB("array")("char")}, {
        {ByteCode::CALLNATIVE, 0, 1}, {ByteCode::RETA} }},
    { "@format", {TB("array")("char"), TB("array")("object"), TB("array")("char")}, {
        {ByteCode::LOADLA, 0, 0}, {ByteCode::LOADLA, 0, 1}, {ByteCode::CALLNATIVE, 0, 2}, {ByteCode::RETA} }},
    { "@sizeof", {TB("object"), TB("int")}, {
        {ByteCode::LOADLI, 0, 0}, {ByteCode::CALLNATIVE, 0, 3}, {ByteCode::RETI} }},
    { "@throw", {TB("object"), TB("bottom")}, {
        {ByteCode::LOADLA, 0, 0}, {ByteCode::THROW} }},
    { "@index", {TB("array")("char"), TB("int"), TB("char")}, {
        {ByteCode::LOADLA, 0, 0}, {ByteCode::LOADLI, 0, 1}, {ByteCode::LOADI}, {ByteCode::RET} }},
    { "@indexf", {TB("array")(TB("function")("nil")), TB("int"), TB("function")("nil")}, {
        {ByteCode::LOADLA, 0, 0}, {ByteCode::LOADLI, 0, 1}, {ByteCode::LOADIA}, {ByteCode::RETA} }},
};