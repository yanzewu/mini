#include "../mini/type.h"
#include "../mini/symtable.h"
#include "../mini/typeaux.h"

#include "ut.h"

using namespace mini;

// typing relationship
void test_typing() {

    SymbolTable symtable;
    symtable.initialize(NULL);
    BuiltinSymbolGenerator::generate_builtin_types(symtable);

    using tb = PrimitiveTypeBuilder;
    using stb = StructTypeBuilder;
    using utb = UniversalTypeBuilder;
    using vtb = TypeVariableBuilder;

    // Primitives

    REQUIRE(Type::compare(tb("nil")().get(), tb("int")().get()) == Type::Ordering::UNCOMPARABLE, "nil != int");
    REQUIRE(Type::compare(tb("int")().get(), tb("int")().get()) == Type::Ordering::EQUAL, "int == int");
    REQUIRE(Type::compare(tb("int")().get(), tb("array")("char")().get()) == Type::Ordering::UNCOMPARABLE, "int != array(char)");
    REQUIRE(Type::compare(tb("array")("int")().get(), tb("array")("char")().get()) == Type::Ordering::UNCOMPARABLE, "array(int) != array(char)");
    REQUIRE(Type::compare(tb("tuple")("int")("int")().get(), 
        tb("tuple")(tb("array")("int"))("int")().get()) == Type::Ordering::UNCOMPARABLE, 
        "tuple(int, int) != tuple(array(int), int)");
    REQUIRE(Type::compare(tb("tuple")(tb("array")("int"))("int")().get(),
        tb("tuple")(tb("array")("int"))("int")().get()) == Type::Ordering::EQUAL,
        "tuple(array(int), int) == tuple(array(int), int)");
    REQUIRE(Type::compare(tb("tuple")(tb("array")("int"))("int")().get(),
        tb("tuple")(tb("array")("top"))("int")().get()) == Type::Ordering::LESS,
        "tuple(array(int), int) == tuple(array(int), int)");
    REQUIRE(Type::compare(tb("bottom")().get(), tb("int")().get()) == Type::Ordering::LESS, "bottom < array(int)");
    REQUIRE(Type::compare(tb("int")().get(), tb("top")().get()) == Type::Ordering::LESS, "int <: top");
    REQUIRE(Type::compare(tb("top")().get(), tb("int")().get()) == Type::Ordering::GREATER, "top :> int");
    REQUIRE(Type::compare(tb("top")().get(), stb()("a", tb("nil"))().get()) == Type::Ordering::GREATER, "top :> struct(a=nil)");
    REQUIRE(Type::compare(stb()().get(), stb()("a", tb("nil"))().get()) == Type::Ordering::GREATER, "struct().get() > struct(a=nil)");
    REQUIRE(Type::compare(stb()("a", tb("int"))().get(), stb()("a", tb("nil"))().get()) == Type::Ordering::UNCOMPARABLE,
        "struct(a=int) != struct(a=nil)");
    REQUIRE(Type::compare(stb()("a", stb()("x", tb("int")))("b", tb("float"))().get(),
        stb()("b", tb("float"))("a", stb()("x", tb("int")))().get()) == Type::Ordering::EQUAL,
        "struct(a=struct(x=int), b=float) = struct(b=float, a=struct(x=int))");
    REQUIRE(Type::compare(tb("function")("int")("top")().get(), tb("function")("int")("int")().get()) == Type::Ordering::GREATER, 
        "function(int, top) :> function(int, int)");
    REQUIRE(Type::compare(tb("function")("top")("top")().get(), tb("function")("int")("top")().get()) == Type::Ordering::UNCOMPARABLE, 
        "function(top, top) != function(int, top)");

    // Match

    REQUIRE(stb()()->as<StructType>()->is_interface_of(stb()("a", tb("int"))().get()), "struct(a=int) <* struct()");
    REQUIRE(stb()("b", tb("float"))("a", stb()("x", tb("top")))()->as<StructType>()->is_interface_of(
        stb()("a", stb()("x", tb("int")))("b", tb("float"))().get()), "struct(a=int) <* struct()");
    REQUIRE(tb("atomic")()->is_interface_of(tb("nil")().get()), "nil <* atomic");
    REQUIRE(tb("atomic")()->is_interface_of(tb("nil")().get()), "int <* atomic");
    REQUIRE(!tb("number")()->is_interface_of(tb("nil")().get()), "nil !<* number");
    REQUIRE(tb("top")()->is_interface_of(stb()("a", tb("int"))().get()), "struct(a=int) <* top");
    REQUIRE(tb("top")()->is_interface_of(utb()("x")(tb("function")(vtb("x"))(vtb("x"))).build(symtable).get()), "forall<x>.function(x,x) <* top");

    // Universal Types

    REQUIRE(Type::compare(utb()("x")(tb("function")(vtb("x"))(vtb("x"))).build(symtable).get(),
        utb()("y")(tb("function")(vtb("y"))(vtb("y"))).build(symtable).get()) == Type::Ordering::EQUAL, "forall<x>.function(x,x) = forall<y>.function(y,y)");
    REQUIRE(Type::compare(utb()("x")(tb("function")(vtb("x"))(tb("top"))).build(symtable).get(),
        utb()("y")(tb("function")(vtb("y"))(tb("int"))).build(symtable).get()) == Type::Ordering::GREATER, "forall<x>.function(x,top) :> forall<x>.function(x,int)");
    REQUIRE(Type::compare(utb()("x")(tb("function")(vtb("x"))(tb("top"))).build(symtable).get(),
        utb()("y")(tb("function")(vtb("y"))(vtb("y"))).build(symtable).get()) == Type::Ordering::GREATER, "forall<x>.function(x,top) :> forall<x>.function(x,x)");
    REQUIRE(Type::compare(utb()("x")(tb("function")(vtb("x"))(vtb("x"))).build(symtable).get(),
        tb("top")().get()) == Type::Ordering::LESS, "forall<x>.function(x,x) <: top");
    REQUIRE(Type::compare(utb()("x")(tb("function")(vtb("x"))(vtb("x"))).build(symtable).get(),
        tb("bottom")().get()) == Type::Ordering::GREATER, "bottom <: forall<x>.function(x,x)");

    // Evaluation

    REQUIRE(utb()("x")("y", tb("atomic"))
        (tb("function")(vtb("x"))(vtb("y"))(tb("tuple")(vtb("x"))(vtb("y")))).build(symtable)->as<UniversalType>()->instanitiate(
            { tb("int")(), tb("nil")() }, SymbolInfo())->equals(
                tb("function")(tb("int"))(tb("nil"))(tb("tuple")("int")("nil"))().get()
            )
        , "evaluate[forall<x,y implements atomic>.function(x,y,tuple(x,y))] with [x=int, y=nil] == function(int,nil,tuple(int,nil))");
    REQUIRE(utb()("x")(tb("function")(vtb("x"))(vtb("x"))).build(symtable)->as<UniversalType>()->instanitiate(
        { utb()("x")(tb("function")(vtb("x"))(vtb("x"))).build(symtable) }, SymbolInfo())->equals(
            tb("function")
            (utb()("x")(tb("function")(vtb("x"))(vtb("x"))))
            (utb()("x")(tb("function")(vtb("x"))(vtb("x")))).build(symtable).get()
        ), "evaluate[forall<x>.function(x,x)] with [x=forall<x>.function(x,x)] == function(forall<x>.function(x,x),forall<x>.function(x,x))");
    REQUIRE(
        utb()("x", stb()("a", tb("int")))(tb("function")("int")(vtb("x"))).build(symtable)->as<UniversalType>()->instanitiate(
            { stb()("a", tb("int"))("b", tb("nil"))() }, SymbolInfo())->equals(
                tb("function")("int")(stb()("a", tb("int"))("b", tb("nil")))().get()),
        "evaluate[forall<x implements struct(a=int)>.function(int,x)] with [x=struct(a=int, b=nil)] == function(int,struct(a=int,b=nil))"
    );
    REQUIRE(utb()("x")(
        tb("function")(utb()("y")(tb("function")(vtb("x"))(vtb("y"))))(vtb("x"))).build(symtable)->as<UniversalType>()->instanitiate(
            { tb("int")() }, SymbolInfo())->equals(
                tb("function")(utb()("y")(tb("function")("int")(vtb("y"))))("int").build(symtable).get()
            ), "evaluate[forall<x>.function(forall<y>.function(x,y),x)] with [x=int] == function(forall<y>.function(int,y),int)");

    summary();
}
