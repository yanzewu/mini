
# Mini: Language Specification

v0.2

## 1. Overview

### 1.1 Interpretation

Mini handles file as input. There is no plan for interactive mode in recent versions.

### 1.2 Statements

There are only two kinds of statements in Mini. One is "control statement", include declaration, definition, assignment, import, etc.. These statements begin with a single keyword, and are often separated with spaces:

    let a:int;
    set b = a;

The other type of statement is function call, which does all the interesting stuff. Different from most languages, Mini does not have operators therefore it is syntactically simple (that's why it gets the name).
All the statements must end with semi-colon `;`.

### 1.3 Modules
To import all the functions and types from module, use `import`. Example:

    import module;
    import a.b.c;

The name of module is the filename without suffix. Relative import from subdirectory is supported, where the dot `.` is the path splitter. In this version, `import` is treated syntactically and namespace is not supported yet.

The default search path is the current directory and installation path. A system library `sys` is provided for basic functions.


## 2. Basic Types

### 2.1 Overview

Intrinsic type deriving graph:

    top
      +---- (atom)
      |       +---- nil
      |       +---- bool
      |       +---- char
      |       +---- int
      |       +---- float
      |
      +---- (list)
              +---- tuple(T1, T2, ...)
              +---- array(T)
              +---- function(T1, T2, ..., Tret)
              +---- {f1=T1, f2=T2, ...}
              +---- object
                    +--- all custom classes
                        +--- bottom

All the atomic types cannot be inherited.

Each type (include `list`) corresponds to a literal.

- nil: `nil`, `()`
- bool: `true`/`false`
- int: integers
- float: float numbers
- char: `'a' 'b'`...
- tuple: `(a, b, ...)`
- function: closure
- expr: statement to be executed
- array: `[a, b, ...]`
- struct: `{x=a, y=b, ...}`

String literal is mapped to `array(char)`. To declare a integral float number, use format like "1.0".

### 2.2 Individual Types

#### top

Top is the root type, which is not intended to be used directly.

#### atom

Instance of atomic cannot be created. It remains conceptual.

#### nil

The empty type `nil` is for empty container and function returns.

#### list

`list` is not a generic type -- it has no type argument. It mainly serves as a type hack and future compatibility with Lisp.

#### array

`array` is a generic type. It takes one type argument and hold objects with same type. So `array(int)` and `array(char)` are different types.

#### tuple

`tuple` behaves like product type. It needs at least one argument:

    let a:tuple(int, int) = (1, 2);

#### struct

`struct` is the recording type, which cannot be defined directly. They can be created as instances:

    {a=1, b=2.0, c='3'}     // {a=int, b=float, c=char}

Named struct can be created through interfaces.

#### function

`function` is the type for closure (either named/anoymous). Variables with function type can be assigned and passed just like other types. A specific function may have a parametrized type

    function(T1, T2, ... Tn, Tret)

which represents a function accepting argument with type `T1`, ... , `Tn` by order and returns `Tret`.

#### bottom

`bottom` is the child type of any types, and is reserved for error handling and compiling (such as the returning value of `throw`).


## 3. Variable System

### 3.1 Let Declaration

The syntax of variable definition is

    let [varname](:type) (= initialize-expr);   // define a variable

The name of variable should not begin with 0-9 and reserved operators (like "()", "<>", "=") and must not coincide with reserved keywords. Variables starting with "@" will be treated as system symbols and thus also not allowed for declaration. It is strongly recommended to use letter at the beginning of variable and letter/number inside variable.

The `initialize-expr` is optional, and should yield an instance with the declared type or its child type. The new variable will have the declared type.

If the declared type is missing, then the `initialize-expr` cannot be omitted, and the variable will have the same type of the expression.

The value of uninitialized variable is unspecified. The variable to be declared must not appear in the initialization expression. However, a variable with same name may be used if it is declared in the outer scope, and the newly declared variable will shadow the previous variable.

If a variable is defined globally, it will have a global scope and can be used in any functions. If a variable is defined inside function, its access is limited in that function. A variable cannot be defined twice within the same scope.

Examples:

    let a:int;                # define a int
    let a:int = 2;            # define a int equals 2
    let b = add(a,1);         # define a int b equals a+1
    let l:tuple(int,int,int) = (1,2,3);     # define a tuple

    class Foo {a:int, b:int};
    let foo1:Foo = {a=1, b=2};  # foo {a=1, b=2}
    let foo2:Foo = {b=2};  # foo {a=unspecified, b=2}


### 3.2 Variable Assignment

The syntax of assignment is

    set [varname] = [expr];

Note the equal operator `=` only has syntactical meaning. The `varname` and `field` cannot be expressions.
More examples:

    let a:int;
    set a = 1;          # a = 1
    set a = add(a,2);   # a = 3
    set a = '1';        # Error: Type not match (int != char)

    interface Foo {a:int, b:int};
    let foo1:Foo = {a=1, b=2};  # foo1 {a=1, b=2}

Assignment of functions is also allowed, as long as their signature matches.

    set myadd = add;        # OK
    set myadd = opposite;   # Error: type not match (function(int,int,int) != function(int,int))

### 3.3 Variable Storation

Variables with the following types are matching (and deriving) `Addressable` and are stored by references:

- Primitive types deriving `list` (including arrays, tuples and functions);
- Objects;
- Structs;
- Universal types satisfy rules above after type erasure (See Section 6.1);

The atomic types are stored by value.

## 4. Functions

### 4.1 Closure Objects

A closure is represented by a capture list and a statement list (a single statement is treated as a list with one element). The general syntax is

    \(arg1:T1, arg2:T2, ..., argn:Tn)->{stat1, stat2, ..., statn}:Tret
    \arg1:T1->stat:Tret
    \()->{stat1,...,statn}:Tret
    
The capture list can either be a list of names or a single name. A function takes no argument has signature like `function(Tret)`. 

The body of function is either a single statement, or a list of statements  separated by `,`. Function body cannot be empty. The last statement of function will be returned and hence it must be an expression. A function returns `nil` should put an expression that returns a `nil` in the end.

The type annotation `Tret` can be omitted, in this case the function will have the same returning type as the last expression. However, in the case of multiple lambdas, the type annotation will be greedy matched, therefore the annotation of the inner function cannot be omitted if the outer function has annotations.

Examples:

    let f = \(a:int, b:int)->add(a, b):int;
    let gaussian = \(x:float)->{
        let y:float = multiply(x, x), 
        exp(negative(y))};

    let id = \x:top->x;        # Identity
    let myadd:function(int, int, int) = add;     # Define a function f which equals add


### 4.2 Def Declaration

Alternatively, `def` provides a syntactic sugar for function definition:

    def bar (var1:type1, var2:type2)->type3  {FUNCTION BLOCK};

The statement above is equivalent to

    let bar = \(var1:type1,var2:type2)->{FUNCTION BLOCK}:type3;

Define gaussian:

    def gaussian(x:float)->float {
        let y:float = multiply(x, x), 
        exp(negative(y))
    };

Function definition can be nested. The scope of defined function is same as the scope of its context - a global function can access global variables, and a local-defined function can access both local variables and variables in outer scope. However, a local function cannot change the value of binding variables (but can change elements of `Addressable` objects). `set` will raise error for such assignment.


## 5. Interfaces

### 5.1 Overview

Interfaces are aliases for structure types and existential types. Different interfaces equal to each other if their fields are the same.

For example, the following syntax declares an interface:

    interface IFooable {
        foo: int
    };

Members must not be initialized in the declaration. An empty interface is equivalent to `object`.
Variables may have interface types:

    let foo:Fooable = {foo=2};

Interface can be syntactically inherited by another interface: Inheriting an interface is equivalent to copying all of its fields and the fields it inherites:

    interface IBar extends IFooable {
        bar:int
    };
    let bar:IBar = {foo=2, bar=3};

It is also possible to inherit from multiple interfaces, as long as their members does not conflict. If two members have same name and type, only _one copy_ will be kept.

    interface IFooBar extends IFooable,IBar {

    };  # equivalent to {foo=int, bar=int}

To specify an interface for a class, use `implements`:

    class Bar implements Fooable {
        foo: int
    };

The type relationship between interfaces and classes are only determined by their fields, rather than the explicit declaration. However, if a class does not satisfy the structural relationship as it has declared, the compilation will fail.


### 5.2 Matching

Different from objects, Mini does not allow any subtypings associated with structure fields. Therefore, variables declared with interfacial types need to have the same fields in assignments. Assignment by variable with a different type is not allowed, even though they are structural subtypes:

    class Base { ... };
    class Derived extends Base { ... };
    interface IFoo { foo:Base };

    let foo:IFoo = {foo=new Base()}     # OK. 
    let foo:IFoo = {foo=new Derived()}  # OK. {foo=Derived} < {foo=Base}
    let foo:IFoo = {foo=new Base(), bar=2};  # Error: type not match: {foo=Base, bar=int} != {foo=Base}

For polymorphism, Mini introduces a new kind of relationship, matching, which is similar to the type classes in Haskell.

The matching relation is mainly used in two senarios: (1) An object implementing an interface by the keyword `implements`; (2) Bounded quantifiers in generic functions. In either case, the RHS must be an interface. To check if LHS matches RHS, the following rule are used:

If LHS and derived class are non-recursive, the fields are checked structurally.


## 6. Generics

### 6.1 Universal Types

The syntax to declare a universal type is

    forall<T1:Q1, T2:Q2, ...>.TYPE_EXPRESSION

`Q1,Q2` are bounded qualifications, and defaults to a virtual `Addressable` if omitted. `Q` may only be interfaces or `Addressable`.
For example, the identify function has signature

    id: forall<T>.function(T,T)

The following syntax defines a polymorphic closure and assign it to variable `f`:

    let f:forall<T1,T2...>.function(T1,T2,...,Tret) = 
        \<T1,T2,...>.(x:TYPE1, y:TYPE2, ...)->{CLOSURE BODY};

Note that a polymorphic non-parametric type will always be bottom:

    let a:forall<T>.T;
    set a = 3;  # Error: Type not match: int and forall<T>.T

To avoid ambiguity, we do not allow nested definition of universal type, i.e. no quantifiers are allowed in `TYPE_EXPRESSION`. This does not harm any expressiveness, since it can always be resolved via currying and de-currying. For example,

    let f = \<T1,T2>.(x:T1, y:T2)->g<T2>(y);   # de-currying
    let f2 = \<T2>(y:T2)->f<int,T2>(2, y);      # currying

When a universal function is applied to actual parameters, all universal quantifiers must be instantiated:

    id<Int>({a=2}); # gives {a=2}
    id({a=2});      # ERROR: parameter need to be instanitiated

 However, this is not necessary for type parameters, since the universal quantifiers are not for function itself:

    let id_pair =
        \(f:forall<x>.function(x,x), a:int, b:char)->(f<Int>(a), f<Char>(b));
    id_pair(id, 2, 'a');        # gives (2, 'a')
    id_pair(id<Int>, 2, 'a');   # Error: type does not match: forall<x>.function(x,x) != function(int,int)


## 7. Predefined Types and Variables

### 7.1 Low-level Functions

Functions that operates directly on primitive types. These functions either cannot be expressed by other functions, or is much slower without native implmentation. All the names of functions start with '@'.

Function | Type | Note
--- | --- | ---
@addi | `function(int,int,int)`
@subi | `function(int,int,int)`
@muli | `function(int,int,int)`
@divi | `function(int,int,int)`
@modi | `function(int,int,int)`
@remi | `function(int,int,int)`
@powi | `function(int,int,int)`
@sqrti | `function(int,int,int)`
@negi | `function(int,int)`
@addf | `function(float,float,float)`
@subf | `function(float,float,float)`
@mulf | `function(float,float,float)`
@divf | `function(float,float,float)`
@modf | `function(float,float,float)`
@remf | `function(float,float,float)`
@powf | `function(float,float,float)`
@sqrtf | `function(float,float,float)`
@negf | `function(float,float)`
@bit_and | `function(int,int,int)`
@bit_or | `function(int,int,int)`
@bit_not | `function(int,int)`
@bit_xor | `function(int,int,int)`
@gei | `function(int,int,bool)`
@gti | `function(int,int,bool)`
@lei | `function(int,int,bool)`
@lti | `function(int,int,bool)`
@eqi | `function(int,int,bool)`
@nei | `function(int,int,bool)`
@gef | `function(float,float,bool)`
@gtf | `function(float,float,bool)`
@lef | `function(float,float,bool)`
@ltf | `function(float,float,bool)`
@eqf | `function(float,float,bool)`
@nef | `function(float,float,bool)`
@print | `function(array(char), nil)`
@input | `function(array(char))`
@format | `function(array(char), array(Addressable), array(char))`
@throw | `function(array(char), bottom)`
@len | `function(Addressable,int)`
@ageti | `function(array(int),int,int)`
@aseti | `function(array(int),int,int,nil)`
@agetf | `function(array(float),int,float)`
@asetf | `function(array(float),int,float,nil)`
@aget | `function(array(char),int,char)`
@aset | `function(array(char),int,char,nil)`
@ageta | `function(array(list),int,list)`
@aseta | `function(array(list),int,list,nil)`
@bool | `function(top,bool)` | converting anything other than 0,0.0f,false to true
@ftoi | `function(float,int)`
@ctoi | `function(char,int)`
@itof | `function(int,float)`
@itoc | `function(int,char)`

### 7.2 Extended Functions

Arithemetic and basic functional programming. Most of them are defined in module std.

Function | Type | Usage
--- | --- | ---
id | `forall<X>.function(X,X)` | identity
const | `forall<X,Y>.function(X,Y,X)` | return the first one
seq | `forall<X,Y>.function(X,Y,Y)` | return the second one
dot | `forall<X,Y,Z>.function(function(X,Y),` `function(Y,Z),function(X,Z))` | function product
flip |`forall<X,Y,Z>.function(function(X,Y,Z),` `function(Y,X,Z))` | flip the first and second argument
curry | `forall<X,Y,Z>.function(function(X,Y,Z),` `function(X,function(Y,Z)))`
uncurry | `forall<X,Y,Z>.function(function(X,function(Y,Z)),` `function(X,Y,Z))`
error | `function(array(char),bottom)` | same as @throw
undefined | `function(bottom)` | @throw("undefined")

Arrays and lists:

Function | Type | Usage
--- | --- | ---
aget | `forall<X>.function(array(X),int,X)`

