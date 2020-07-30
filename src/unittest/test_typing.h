#include "../mini/type.h"
#include "../mini/symtable.h"
#include "../mini/builtin.h"

#include "ut.h"

using namespace mini;

// typing relationship
void test_typing() {

    SymbolTable symtable;
    symtable.initialize(NULL);
    BuiltinSymbolGenerator::generate_builtin_types(symtable);

    using TB = BuiltinTypeBuilder;

    REQUIRE(Type::compare(*TB("nil")(), *TB("int")()) == Type::Ordering::UNCOMPARABLE, "nil != int");
    REQUIRE(Type::compare(*TB("int")(), *TB("int")()) == Type::Ordering::EQUAL, "int == int");
    REQUIRE(Type::compare(*TB("int")(), *TB("array")("char")()) == Type::Ordering::UNCOMPARABLE, "int != array(char)");
    REQUIRE(Type::compare(*TB("array")("int")(), *TB("array")("char")()) == Type::Ordering::UNCOMPARABLE, 
        "array(int) != array(char)");
    REQUIRE(Type::compare(*TB("tuple")("int")("int")(), 
        *TB("tuple")(TB("array")("int"))("int")()) == Type::Ordering::UNCOMPARABLE, 
        "tuple(int, int) != tuple(array(int), int)");
    REQUIRE(Type::compare(*TB("tuple")(TB("array")("int"))("int")(),
        *TB("tuple")(TB("array")("int"))("int")()) == Type::Ordering::EQUAL,
        "tuple(array(int), int) == tuple(array(int), int)");
    REQUIRE(Type::compare(*TB("tuple")(TB("array")("int"))("int")(),
        *TB("tuple")(TB("array")("object"))("int")()) == Type::Ordering::LESS,
        "tuple(array(int), int) == tuple(array(int), int)");
    REQUIRE(Type::compare(*TB("bottom")(), *TB("int")()) == Type::Ordering::LESS, "bottom < array(int)");
    REQUIRE(Type::compare(*TB("int")(), *TB("object")()) == Type::Ordering::LESS, 
        "int <: object");
    REQUIRE(Type::compare(*TB("object")(), *TB("int")()) == Type::Ordering::GREATER, 
        "object :> int");
    REQUIRE(Type::compare(*TB("object")(), *TB("struct")("a", "nil")()) == Type::Ordering::GREATER,
        "object :> struct(a=nil)");
    REQUIRE(Type::compare(*TB("struct")(), *TB("struct")("a", "nil")()) == Type::Ordering::GREATER,
        "struct() > struct(a=nil)");
    REQUIRE(Type::compare(*TB("struct")("a", "int")(), *TB("struct")("a", "nil")()) == Type::Ordering::UNCOMPARABLE,
        "struct(a=int) != struct(a=nil)");
    REQUIRE(Type::compare(*TB("struct")("a", TB("struct")("x", "int"))("b", "float")(),
        *TB("struct")("b", "float")("a", TB("struct")("x", "int"))()) == Type::Ordering::EQUAL,
        "struct(a=struct(x=int), b=float) = struct(b=float, a=struct(x=int))");
    REQUIRE(Type::compare(*TB("function")("int")("object")(), *TB("function")("int")("int")()) == Type::Ordering::GREATER, 
        "function(int, object) :> function(int, int)");
    REQUIRE(Type::compare(*TB("function")("object")("object")(), *TB("function")("int")("object")()) == Type::Ordering::UNCOMPARABLE, 
        "function(object, object) != function(int, object)");
    summary();
}
