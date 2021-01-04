#include "ut.h"
#include "../mini/mini.h"

using namespace mini;

void parse(const std::string& str, std::vector<Token>& token_buffer, std::vector<pAST>& buffer) {
    mini::Lexer lexer;
    mini::Parser parser;

    token_buffer.clear();
    buffer.clear();

    lexer.tokenize(str, token_buffer, 0);
    parser.parse(token_buffer, buffer);
}


void test_parser() {

    std::vector<Token> token_buffer;
    std::vector<pAST> ast;
    std::string s;

#define PARSE(x) s = x; parse(s, token_buffer, ast)

    PARSE("import a; ; ; "); REQUIRE(ast[0]->as<ImportNode>()->get_filename() == "a", s.c_str());
    PARSE("import a.b.c.d;"); REQUIRE(ast[0]->as<ImportNode>()->get_filename() == "a/b/c/d", s.c_str());

    // Literals

    PARSE("let v1:int;"); REQUIRE(ast_cast<LetNode>(ast[0])->symbol->get_name() == "v1", s.c_str());
                        REQUIRE(ast_cast<LetNode>(ast[0])->vtype->symbol->get_name() == "int", s.c_str());
    PARSE("let v2:int = 10;"); REQUIRE(ast_cast<ConstantNode>(ast_cast<LetNode>(ast[0])->expr)->value == Constant(10), s.c_str());
    PARSE("let v3:float = 10.23;"); REQUIRE(ast_cast<ConstantNode>(ast_cast<LetNode>(ast[0])->expr)->value == Constant(10.23f), s.c_str());
    PARSE("let v4:char = '1';"); REQUIRE(ast_cast<ConstantNode>(ast_cast<LetNode>(ast[0])->expr)->value == Constant('1'), s.c_str());
    PARSE("let v4_2:char = '\\n';"); REQUIRE(ast_cast<ConstantNode>(ast_cast<LetNode>(ast[0])->expr)->value == Constant('\n'), s.c_str());
    PARSE("let v5:bool = true;"); REQUIRE(ast_cast<ConstantNode>(ast_cast<LetNode>(ast[0])->expr)->value == Constant(true), s.c_str());
    PARSE("let v6:nil = nil;"); REQUIRE(ast_cast<ConstantNode>(ast_cast<LetNode>(ast[0])->expr)->value == Constant(), s.c_str());
    PARSE("let v6_2:nil = ();"); REQUIRE(ast_cast<ConstantNode>(ast_cast<LetNode>(ast[0])->expr)->value == Constant(), s.c_str());
    PARSE("let v6_3:nil = (());"); REQUIRE(ast_cast<ConstantNode>(ast_cast<LetNode>(ast[0])->expr)->value == Constant(), s.c_str());
    PARSE("let v7:array(char) = \"12345\";"); REQUIRE(ast_cast<ConstantNode>(ast_cast<LetNode>(ast[0])->expr)->value == Constant("12345"), s.c_str());

    PARSE("(1,2,3);"); 
        REQUIRE(ast_cast<ConstantNode>(ast_cast<TupleNode>(ast[0])->children[0])->value == Constant(1), s.c_str());
        REQUIRE(ast_cast<ConstantNode>(ast_cast<TupleNode>(ast[0])->children[1])->value == Constant(2), s.c_str());
        REQUIRE(ast_cast<ConstantNode>(ast_cast<TupleNode>(ast[0])->children[2])->value == Constant(3), s.c_str());
    PARSE("let v8_2:tuple(int,char,bool,nil,nil,array(char),tuple(int,int,int)) = (1, 'a', true, nil, (), \"1234\", v8);");
        REQUIRE(ast_cast<ConstantNode>(ast_cast<TupleNode>(ast_cast<LetNode>(ast[0])->expr)->children[0])->value == Constant(1), s.c_str());
        REQUIRE(ast_cast<ConstantNode>(ast_cast<TupleNode>(ast_cast<LetNode>(ast[0])->expr)->children[1])->value == Constant('a'), s.c_str());
        REQUIRE(ast_cast<ConstantNode>(ast_cast<TupleNode>(ast_cast<LetNode>(ast[0])->expr)->children[2])->value == Constant(true), s.c_str());
        REQUIRE(ast_cast<ConstantNode>(ast_cast<TupleNode>(ast_cast<LetNode>(ast[0])->expr)->children[3])->value == Constant(), s.c_str());
        REQUIRE(ast_cast<ConstantNode>(ast_cast<TupleNode>(ast_cast<LetNode>(ast[0])->expr)->children[4])->value == Constant(), s.c_str());
        REQUIRE(ast_cast<ConstantNode>(ast_cast<TupleNode>(ast_cast<LetNode>(ast[0])->expr)->children[5])->value == Constant("1234"), s.c_str());
        REQUIRE(ast_cast<VarNode>(ast_cast<TupleNode>(ast_cast<LetNode>(ast[0])->expr)->children[6])->symbol->get_name() == "v8", s.c_str());
        REQUIRE(ast_cast<TypeNode>(ast_cast<LetNode>(ast[0])->vtype)->symbol->get_name() == "tuple", s.c_str());
        REQUIRE(ast_cast<TypeNode>(ast_cast<LetNode>(ast[0])->vtype)->args[0]->symbol->get_name() == "int", s.c_str());
        REQUIRE(ast_cast<TypeNode>(ast_cast<LetNode>(ast[0])->vtype)->args[1]->symbol->get_name() == "char", s.c_str());
        REQUIRE(ast_cast<TypeNode>(ast_cast<LetNode>(ast[0])->vtype)->args[2]->symbol->get_name() == "bool", s.c_str());
        REQUIRE(ast_cast<TypeNode>(ast_cast<LetNode>(ast[0])->vtype)->args[3]->symbol->get_name() == "nil", s.c_str());
        REQUIRE(ast_cast<TypeNode>(ast_cast<LetNode>(ast[0])->vtype)->args[4]->symbol->get_name() == "nil", s.c_str());
        REQUIRE(ast_cast<TypeNode>(ast_cast<LetNode>(ast[0])->vtype)->args[5]->symbol->get_name() == "array", s.c_str());
        REQUIRE(ast_cast<TypeNode>(ast_cast<LetNode>(ast[0])->vtype)->args[6]->symbol->get_name() == "tuple", s.c_str());
        REQUIRE(ast_cast<TypeNode>(ast_cast<LetNode>(ast[0])->vtype)->args[6]->args[0]->symbol->get_name() == "int", s.c_str());
        REQUIRE(ast_cast<TypeNode>(ast_cast<LetNode>(ast[0])->vtype)->args[6]->args[1]->symbol->get_name() == "int", s.c_str());
        REQUIRE(ast_cast<TypeNode>(ast_cast<LetNode>(ast[0])->vtype)->args[6]->args[2]->symbol->get_name() == "int", s.c_str());
    PARSE("(((1,2),(3),()));");
        REQUIRE(ast_cast<ConstantNode>(ast_cast<TupleNode>(ast_cast<TupleNode>(ast[0])->children[0])->children[0])->value == Constant(1), s.c_str());
        REQUIRE(ast_cast<ConstantNode>(ast_cast<TupleNode>(ast_cast<TupleNode>(ast[0])->children[0])->children[1])->value == Constant(2), s.c_str());
        REQUIRE(ast_cast<ConstantNode>(ast_cast<TupleNode>(ast[0])->children[1])->value == Constant(3), s.c_str());
        REQUIRE(ast_cast<ConstantNode>(ast_cast<TupleNode>(ast[0])->children[2])->value == Constant(), s.c_str());

    PARSE("[];");REQUIRE(ast_cast<ArrayNode>(ast[0])->children.size() == 0, s.c_str());
    PARSE("[1];");REQUIRE(ast_cast<ConstantNode>(ast_cast<ArrayNode>(ast[0])->children[0])->value == Constant(1), s.c_str());
    PARSE("[[], [1]];");REQUIRE(ast_cast<ArrayNode>(ast_cast<ArrayNode>(ast[0])->children[0])->children.size() == 0, s.c_str());
        REQUIRE(ast_cast<ConstantNode>(ast_cast<ArrayNode>(ast_cast<ArrayNode>(ast[0])->children[1])->children[0])->value == Constant(1), s.c_str());

    PARSE("{a={}, b=({}), c={a=1}};");
        REQUIRE(ast_cast<StructNode>(ast[0])->children[0].first->get_name() == "a", s.c_str());
        REQUIRE(ast_cast<StructNode>(ast[0])->children[1].first->get_name() == "b", s.c_str());
        REQUIRE(ast_cast<StructNode>(ast[0])->children[2].first->get_name() == "c", s.c_str());
        REQUIRE(ast_cast<StructNode>(ast_cast<StructNode>(ast[0])->children[0].second)->children.size() == 0, s.c_str());
        REQUIRE(ast_cast<StructNode>(ast_cast<StructNode>(ast[0])->children[1].second)->children.size() == 0, s.c_str());
        REQUIRE(ast_cast<StructNode>(ast_cast<StructNode>(ast[0])->children[2].second)->children[0].first->get_name() == "a", s.c_str());

    PARSE("index([add, \\(a:int, b:int)->sub(a,b):int], 0)(1, 2);");
        REQUIRE(ast[0]->as<FunCallNode>()->caller->as<FunCallNode>()->args[1]->as<ConstantNode>()->value == Constant(0), s.c_str());
        REQUIRE(ast[0]->as<FunCallNode>()->caller->as<FunCallNode>()->args[0]->as<ArrayNode>()->children[0]->as<VarNode>()->symbol->get_name() == "add", s.c_str());
        REQUIRE(ast[0]->as<FunCallNode>()->caller->as<FunCallNode>()->args[0]->as<ArrayNode>()->children[1]->as<LambdaNode>()->statements[0]->
            as<FunCallNode>()->caller->as<VarNode>()->symbol->get_name() == "sub", s.c_str());
    PARSE("set e2 = e3;"); REQUIRE(ast[0]->as<SetNode>()->lhs->as<VarNode>()->symbol->get_name() == "e2", s.c_str());

    // GetField

    PARSE("obj.method(1,2)(3,4)((5,6));");
        REQUIRE(ast[0]->as<FunCallNode>()->args[0]->as<TupleNode>()->children[1]->as<ConstantNode>()->value == Constant(6), s.c_str());
        REQUIRE(ast[0]->as<FunCallNode>()->caller->as<FunCallNode>()->args[1]->as<ConstantNode>()->value == Constant(4), s.c_str());
        REQUIRE(ast[0]->as<FunCallNode>()->caller->as<FunCallNode>()->caller->as<FunCallNode>()->args[1]->as<ConstantNode>()->value == Constant(2), s.c_str());
        REQUIRE(ast[0]->as<FunCallNode>()->caller->as<FunCallNode>()->caller->as<FunCallNode>()->caller->as<GetFieldNode>()->field->get_name() == "method", s.c_str());
        REQUIRE(ast[0]->as<FunCallNode>()->caller->as<FunCallNode>()->caller->as<FunCallNode>()->caller->as<GetFieldNode>()->lhs->as<VarNode>()->symbol->get_name() == "obj", s.c_str());
    PARSE("(expression(obj1, obj2)).method1.method2(obj3, obj4).method3(obj5);");
        REQUIRE(ast[0]->as<FunCallNode>()->args[0]->as<VarNode>()->symbol->get_name() == "obj5", s.c_str());
        REQUIRE(ast[0]->as<FunCallNode>()->caller->as<GetFieldNode>()->field->get_name() == "method3", s.c_str());
        REQUIRE(ast[0]->as<FunCallNode>()->caller->as<GetFieldNode>()->lhs->as<FunCallNode>()->args[1]->as<VarNode>()->symbol->get_name() == "obj4", s.c_str());
        REQUIRE(ast[0]->as<FunCallNode>()->caller->as<GetFieldNode>()->lhs->as<FunCallNode>()->caller->as<GetFieldNode>()->field->get_name() == "method2", s.c_str());
        REQUIRE(ast[0]->as<FunCallNode>()->caller->as<GetFieldNode>()->lhs->as<FunCallNode>()->caller->as<GetFieldNode>()->lhs->as<GetFieldNode>()
            ->field->get_name() == "method1", s.c_str());
        REQUIRE(ast[0]->as<FunCallNode>()->caller->as<GetFieldNode>()->lhs->as<FunCallNode>()->caller->as<GetFieldNode>()->lhs->as<GetFieldNode>()->lhs->as<FunCallNode>()->
            args[1]->as<VarNode>()->symbol->get_name() == "obj2", s.c_str());
        REQUIRE(ast[0]->as<FunCallNode>()->caller->as<GetFieldNode>()->lhs->as<FunCallNode>()->caller->as<GetFieldNode>()->lhs->as<GetFieldNode>()->lhs->as<FunCallNode>()->
            caller->as<VarNode>()->symbol->get_name() == "expression", s.c_str());
    PARSE("set {a=m1, b=m2}.m3(o1, o2).m4 = m5;");
        REQUIRE(ast[0]->as<SetNode>()->lhs->as<GetFieldNode>()->field->get_name() == "m4", s.c_str());
        REQUIRE(ast[0]->as<SetNode>()->lhs->as<GetFieldNode>()->lhs->as<FunCallNode>()->caller->as<GetFieldNode>()->field->get_name() == "m3", s.c_str());
        REQUIRE(ast[0]->as<SetNode>()->lhs->as<GetFieldNode>()->lhs->as<FunCallNode>()->caller->as<GetFieldNode>()->lhs->as<StructNode>()->children[1].first->get_name() == "b", s.c_str());
        REQUIRE(ast[0]->as<SetNode>()->lhs->as<GetFieldNode>()->lhs->as<FunCallNode>()->caller->as<GetFieldNode>()->lhs->as<StructNode>()->children[1].second->as<VarNode>()
            ->symbol->get_name() == "m2", s.c_str());

    // Lambda and Functions

    PARSE("\\x:int->x:int;");
        REQUIRE(ast_cast<LambdaNode>(ast[0])->args[0].first->get_name() == "x", s.c_str());
        REQUIRE(ast_cast<TypeNode>(ast_cast<LambdaNode>(ast[0])->args[0].second)->symbol->get_name() == "int", s.c_str());
        REQUIRE(ast_cast<VarNode>(ast_cast<LambdaNode>(ast[0])->statements[0])->symbol->get_name() == "x", s.c_str());
        REQUIRE(ast_cast<LambdaNode>(ast[0])->ret_type->symbol->get_name() == "int", s.c_str());
    PARSE("\\(x:int)->{x}:int;");
        REQUIRE(ast_cast<LambdaNode>(ast[0])->args[0].first->get_name() == "x", s.c_str());
        REQUIRE(ast_cast<TypeNode>(ast_cast<LambdaNode>(ast[0])->args[0].second)->symbol->get_name() == "int", s.c_str());
        REQUIRE(ast_cast<VarNode>(ast_cast<LambdaNode>(ast[0])->statements[0])->symbol->get_name() == "x", s.c_str());
        REQUIRE(ast_cast<LambdaNode>(ast[0])->ret_type->symbol->get_name() == "int", s.c_str());
    PARSE("\\(x:int)->{{}}:object;");
        REQUIRE(ast_cast<StructNode>(ast_cast<LambdaNode>(ast[0])->statements[0])->children.size() == 0, s.c_str());
    PARSE("\\()->{}:object;");
        REQUIRE(ast_cast<LambdaNode>(ast[0])->args.size() == 0, s.c_str());
        REQUIRE(ast_cast<StructNode>(ast_cast<LambdaNode>(ast[0])->statements[0])->children.size() == 0, s.c_str());
    PARSE("\\(a:int, b:int)->{a,add(a,b)}:object;");
        REQUIRE(ast_cast<LambdaNode>(ast[0])->statements.size() == 2, s.c_str());
    PARSE("\\(a:int, b:int)->{a=a, b=b}:object;");
        REQUIRE(ast_cast<StructNode>(ast_cast<LambdaNode>(ast[0])->statements[0])->children.size() == 2, s.c_str());
    PARSE("def f2<T implements Q>(a:int, b:int)->int {let f2_l1:function(int, int)=\\x:int->x:int, def f2_f1(a:int, b:int)->int {add(a,b)}, 1};");
        REQUIRE(ast_cast<LetNode>(ast[0])->symbol->get_name() == "f2", s.c_str());
        REQUIRE(ast_cast<LetNode>(ast[0])->vtype->symbol->get_name() == "function", s.c_str());
        REQUIRE(ast_cast<LetNode>(ast[0])->vtype->args[0]->symbol->get_name() == "int", s.c_str());
        REQUIRE(ast_cast<LetNode>(ast[0])->vtype->args[1]->symbol->get_name() == "int", s.c_str());
        REQUIRE(ast_cast<LetNode>(ast[0])->vtype->args[2]->symbol->get_name() == "int", s.c_str());
        REQUIRE(ast_cast<LetNode>(ast[0])->vtype->quantifiers[0].second->symbol->get_name() == "Q", s.c_str());
        REQUIRE(ast_cast<LambdaNode>(ast_cast<LetNode>(ast[0])->expr)->args.size() == 2, s.c_str());
        REQUIRE(ast_cast<LambdaNode>(ast_cast<LetNode>(ast[0])->expr)->args[0].first->get_name() == "a", s.c_str());
        REQUIRE(ast_cast<LambdaNode>(ast_cast<LetNode>(ast[0])->expr)->args[0].second->symbol->get_name() == "int", s.c_str());
        REQUIRE(ast_cast<LambdaNode>(ast_cast<LetNode>(ast[0])->expr)->ret_type->symbol->get_name() == "int", s.c_str());
        REQUIRE(ast_cast<LambdaNode>(ast_cast<LetNode>(ast[0])->expr)->statements.size() == 3, s.c_str());
        REQUIRE(ast_cast<LetNode>(ast_cast<LambdaNode>(ast_cast<LetNode>(ast[0])->expr)->statements[1])->symbol->get_name() == "f2_f1", s.c_str());
        REQUIRE(ast_cast<LambdaNode>(ast_cast<LetNode>(ast_cast<LambdaNode>(ast_cast<LetNode>(ast[0])->expr)->statements[1])->expr)->ret_type->symbol->get_name() == "int", s.c_str());

    // Interfaces

    PARSE("interface I1 extends I2,I3 {a:int, b:array(char)};"); 
        REQUIRE(ast_cast<InterfaceNode>(ast[0])->symbol->get_name() == "I1", s.c_str());
        REQUIRE(ast[0]->as<InterfaceNode>()->parents[1]->get_name() == "I3", s.c_str());
        REQUIRE(ast_cast<LetNode>(ast_cast<InterfaceNode>(ast[0])->members[0])->symbol->get_name() == "a", s.c_str());
        REQUIRE(ast_cast<LetNode>(ast_cast<InterfaceNode>(ast[0])->members[1])->symbol->get_name() == "b", s.c_str());

    // Generics

    PARSE("let gv1:forall<T1 implements forall<T>.G(T), T2>.function(T1, T2, forall<X>.Y(X, T1));");
        REQUIRE(ast[0]->as<LetNode>()->vtype->quantifiers[1].first->get_name() == "T2", s.c_str());
        REQUIRE(ast[0]->as<LetNode>()->vtype->quantifiers[0].second->symbol->get_name() == "G", s.c_str());
        REQUIRE(ast[0]->as<LetNode>()->vtype->quantifiers[0].second->quantifiers[0].first->get_name() == "T", s.c_str());
        REQUIRE(ast[0]->as<LetNode>()->vtype->args[2]->quantifiers[0].first->get_name() == "X", s.c_str());
        REQUIRE(ast[0]->as<LetNode>()->vtype->args[2]->symbol->get_name() == "Y", s.c_str());
    PARSE("\\<T2 implements I2>(t2:T2)->\\<X>(x:X)->f<X,T2,int>(x, t2, 2);");
        REQUIRE(ast[0]->as<LambdaNode>()->quantifiers[0].second->symbol->get_name() == "I2", s.c_str());
        REQUIRE(ast[0]->as<LambdaNode>()->statements[0]->as<LambdaNode>()->quantifiers[0].first->get_name() == "X", s.c_str());
        REQUIRE(ast[0]->as<LambdaNode>()->statements[0]->as<LambdaNode>()->statements[0]->as<FunCallNode>()->args[1]->as<VarNode>()->symbol->get_name() == "t2", s.c_str());
        REQUIRE(ast[0]->as<LambdaNode>()->statements[0]->as<LambdaNode>()->statements[0]->as<FunCallNode>()->caller->as<TypeApplNode>()->args[2]->symbol->get_name() == "int", s.c_str());
    PARSE("obj.method<T1,T2>(a<T3>, b)<T4>;");
        REQUIRE(ast[0]->as<TypeApplNode>()->args[0]->symbol->get_name() == "T4", s.c_str());
        REQUIRE(ast[0]->as<TypeApplNode>()->lhs->as<FunCallNode>()->args[0]->as<TypeApplNode>()->args[0]->symbol->get_name() == "T3", s.c_str());
        REQUIRE(ast[0]->as<TypeApplNode>()->lhs->as<FunCallNode>()->caller->as<TypeApplNode>()->args[1]->symbol->get_name() == "T2", s.c_str());

    // Classes

    PARSE("class C1 {m1:int, virtual m2:array(int), new <X,Y>(a:int)->{set self.m1 = a,set self.m2 = [1, 2, 3]} };");
        REQUIRE(ast_cast<LetNode>(ast_cast<ClassNode>(ast[0])->members[0].first)->symbol->get_name() == "m1", s.c_str());
        REQUIRE(ast_cast<LetNode>(ast_cast<ClassNode>(ast[0])->members[1].first)->symbol->get_name() == "m2", s.c_str());
        REQUIRE(ast_cast<ClassNode>(ast[0])->members[1].second.is_virtual, s.c_str());
        REQUIRE(ast[0]->as<ClassNode>()->constructor->quantifiers[0].first->get_name() == "X", s.c_str());
        REQUIRE(ast[0]->as<ClassNode>()->constructor->statements.size() == 2, s.c_str());
        
    PARSE("class C2 extends C1 implements I1 {new(b:int) extends C1<int, char>(b)->{new C1<int, float>(2)}, m3:int };"); 
        REQUIRE(ast_cast<LetNode>(ast_cast<ClassNode>(ast[0])->members[0].first)->symbol->get_name() == "m3", s.c_str());
        REQUIRE(ast_cast<ClassNode>(ast[0])->base->symbol->get_name() == "C1", s.c_str());
        REQUIRE(ast_cast<ClassNode>(ast[0])->interfaces[0]->symbol->get_name() == "I1", s.c_str());
        REQUIRE(ast[0]->as<ClassNode>()->constructor->statements[0]->as<FunCallNode>()->caller->as<NewNode>()->symbol->get_name() == "C1", s.c_str());
        REQUIRE(ast[0]->as<ClassNode>()->constructor->statements[0]->as<FunCallNode>()->caller->as<NewNode>()->type_args[1]->symbol->get_name() == "char", s.c_str());
        REQUIRE(ast[0]->as<ClassNode>()->constructor->statements[1]->as<FunCallNode>()->caller->as<NewNode>()->symbol->get_name() == "C1", s.c_str());
        REQUIRE(ast[0]->as<ClassNode>()->constructor->statements[1]->as<FunCallNode>()->caller->as<NewNode>()->type_args[1]->symbol->get_name() == "float", s.c_str());
        
    summary();

#undef PARSE
}