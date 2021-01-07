
# Mini: Language Specification

v0.2

## 1. Overview

### 1.1 Interpretation

Mini handles file as input. There is no plan for interactive mode in recent versions.

### 1.2 Statements

There are only two kinds of statements in Mini. One is "control statement", include declaration, definition, assignment, import, etc.. These statements begin with a single keyword, and are often separated with spaces:

    let a:int;
    set b = a;
    class Foo {x:int, y:int, new()->{}};

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
    set [expr].[field] = [expr];

Note the equal operator `=` only has syntactical meaning. The `varname` and `field` cannot be expressions.
More examples:

    let a:int;
    set a = 1;          # a = 1
    set a = add(a,2);   # a = 3
    set a = '1';        # Error: Type not match (int != str)

    interface Foo {a:int, b:int};
    let foo1:Foo = {a=1, b=2};  # foo1 {a=1, b=2}
    set foo1.a = 2;             # foo1 {a=2, b=2}

Assignment of functions is also allowed, as long as their signature matches.

    set myadd = add;        # OK
    set myadd = opposite;   # Error: type not match (function(int,int,int) != function(int,int))

### 3.3 Variable Storation

Variables with the following types are matching (and deriving) `Addressable` and are stored by references:

- Primitive types deriving `list` (including arrays, tuples and functions);
- Objects;
- Structs;
- Universal types satisfy rules above after type erasure (See section 7.1);

The atomic types are stored by value. However, there are predefined boxed types `Nil,Bool,Char,Int,Float` for each atomic type, which are just normal objects.

## 4. Functions

### 4.1 Closure Objects

A closure is represented by a capture list and a statement list (a single statement is treated as a list with one element). The general syntax is

    \(arg1:T1, arg2:T2, ..., argn:Tn)->{stat1, stat2, ..., statn}:Tret  # multiple args
    \arg1:T1->stat:Tret             # a single arg
    \()->{stat1,...,statn}:Tret     # no args
    
The type signature of a function is like `function(T1,T2, ..., Tret)`. 

The body of function is either a single statement, or a list of statements  separated by `,`. Function body cannot be empty. The last statement of function will be returned and hence it must be an expression.

The type annotation `Tret` can be omitted, in this case the function will have the same returning type as the last expression. However, in the case of nested lambdas, the type annotation will be greedy matched, therefore the annotation of the inner function cannot be omitted if the outer function has annotations. For example, the following case

    \x:int->\x:int->x:function(int,int)

cannot be parsed since `function(int,int)` will be treated as the type annotation of the inner lambda.

Examples:

    let f = \(a:int, b:int)->add(a, b):int;
    let gaussian = \(x:float)->{
        let y:float = multiply(x, x), 
        exp(negative(y))};

    let id = \x:top->x;        # Identity
    let myadd:function(int, int, int) = add;     # Define a function f which equals add

Function definition can be nested. The scope of defined function is same as the scope of its context - a global function can access global variables, and a local-defined function can access both local variables and variables in outer scope. However, a local function cannot change the value of binding variables (but can change elements of `Addressable` objects). `set` will raise error for such assignment.

Function cannot be recursive. However, it is easily achieved by separating the declaration and assignment (this only works with global variables):

    let f:function(int, int);
    set f = \x:int->f(x);       # note this will be an infinite loop

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



## 5. Object System

The object system in Mini has a similar behavior as in OOP systems. The structural typing is achieved via interfaces.

### 5.1 Class Definitions

Syntax of defining a class:

    class CLASSNAME (extends SUPERCLASS) (implements INTERFACE) {
        DEFINITION BLOCK, ...
        CONSTRUCTOR, ...
    };

The definition block can contains variable definition or function definitions, separated by comma. Example:

    class Foo {
        a:int,
        b:char,
        bar:function(int, int)
        ...
    };

All classes inherit `object`.

### 5.2 Constructors

An object may have one constructor. The syntax for constructors are

    new(arg1:T1, ..., argn:Tn)->{FUNCTION BLOCK}
    new(arg1:T1, ..., argn:Tn) extends SUPERCLASS(EXPR1, EXPR2, ...)->{FUNCTION BLOCK}

The second type calls a constructor of super class (it won't be called by default). If the constructor is not provided, a default one with no argument will be created. Constructors are not class members, but global functions.

Keyword `self` will be available inside the constructor function. The result of last statement in the constructor will not be returned, instead the object instance `self` will be returned.

Example of constructors:

    class Point {
        x:int, y:int, get_r2:function(int),
        new(x:int, y:int)->{
            set self.x = x,
            set self.y = y,
            set self.get_r2 = \()->add(mul(self.x, self.x), mul(self.y, self.y))
        }
    };
    let p = new Point(3, 4);
    p.get_r2();     // gives 25

In the example above, the constructor is equivalent to a global function:

    class Point {...};
    let new_Point = \self:Point->\(x:int, y:int)->{
        ...
        self
    };
    let p = new_Point(# dummy variable of Point #)(3, 4);

Constructors are allowed to be recursive, i.e. constructor of the class can be referred in the definition:

    class Bool {
        val:int,
        equals:function(Bool,Bool),
        new(val:int)->{
            set self.equals = \rhs:Bool->new Bool(@eqi(self.val, rhs.val))
        }
    }

### 5.3 Inheritances

Class can be inherited with `extends`. A class may only have one base class.

    class Point {x:int, y:int};
    class ColoredPoint extends Point {color:tuple(float, float, float)};

The field names in the child classes cannot be same as members in the base class, unless declared with `virtual`:

    class Base {
        virtual print:function(nil),
        new()->{
            let self.print = \()->print("Base class\n")
        }
    };
    class Derived extends Base {
        print:function(nil),    # not necessary here
        new()->{
            let self.print()->print("Derived class\n")
        }
    };
    let b:Base = new Derived();
    b.print();  // "Derived class"

The literal type of virtual members must be same.

### 5.4 Recursive Objects

Similar to imperative languages (such as Java and C++), self-recursion of classes can be achieved without any additional notations. For example:

    class LinkedList {
        val:int,
        next:LinkedList
    };

Since classes are indexed types, the subtyping of recursive objects is checked by the explicit `extends`, which is checked by fields in the declaration. Specifically, to check if B inherits A, we check the unfolded structure of B and A in the assumption of B < A, i.e.

    B < A (assumed), unfold B < unfold A => B < A (globally)

Here unfolding is understood of unrolling classes to structures, either recursive or non-recursive. However, such recursive checking is only done when it is explicitly declared.

The bound checking between objects and structs are checked by slightly different rules. See Section 6.3 for details.


## 6. Interfaces

### 6.1 Overview

Interfaces are aliases for structure types and existential types. Different interfaces equal to each other if their fields are the same.

For example, the following syntax declares an interface:

    interface IFooable {
        foo: int
    };

Members must not be initialized in the declaration. An empty interface is equivalent to `object`.
Variables may have interface types:

    let foo:Fooable = {foo=2};

Different interfaces are same and can be used alternatively if they have the same underlying structure type.

    interface IFooable2 {foo:int};
    let foo2:IFooable2 = foo;

Interface cannot be recursive.

### 6.2 Inheritance

Interface can be syntactically inherited by another interface: Inheriting an interface is equivalent to copying all of its fields and the fields it inherites:

    interface IBar extends IFooable {
        bar:int
    };
    let bar:IBar = {foo=2, bar=3};

It is also possible to inherit from multiple interfaces, as long as their members does not conflict (i.e. has same name but different types). If two members have same name and type, only _one copy_ will be kept.

    interface IFooBar extends IFooable,IBar {

    };  # equivalent to {foo=int, bar=int}

Different from objects, Mini does not allow any subtypings by extending structure fields, since structures can be built easily and the extensive use of up-casting can hide problems. (But this may change in the future) Therefore, variables declared with interfacial types need to have the same fields in assignments:

    class Base { ... };
    class Derived extends Base { ... };
    interface IFoo { foo:Base };

    let foo:IFoo = {foo=new Base()}     # OK. 
    let foo:IFoo = {foo=new Derived()}  # OK. {foo=Derived} < {foo=Base}
    let foo:IFoo = {foo=new Base(), bar=2};  # Error: type not match: {foo=Base, bar=int} != {foo=Base}

Moreover, there is no direct subtyping between structures and objects, since they derives from different base objects (empty struct and `object`, respectively). 

All the structural relationship are expressed by type matching, see next section.

### 6.3 Bound Checking

Bound checking expresses the structrual relationship between structures and structures/objects. It is used in generic functions and types.

Optionally, an object may explicitly implement an interface by the keyword `implements`. The bound checking still passes if the implementation is implicit.

    class Bar implements Fooable {
        foo: int
    };

We define LHS _type matches_ RHS if all RHS's fields are in LHS with types less or equal to that of LHS's fields. 
We require RHS to be a non-recursive structural type (except `Addressable`), so that type matching is a strict extension to structural subtyping. The rules are

1. If LHS is a non-recursive structure type, the fields are checked structurally;
2. If LHS is a recursive structure type or an object, it is unfolded once before comparing;

We do not allow the transistivity of type matching. For types outside objects and classes with kind *, they may only be matched with `Addressable`.

Generic interfaces can also be type matched, even in recursive cases:

    interface Equalable<T> {
        equals: function(T, bool)
    };
    class Integer implements Equalable<Integer> {
        equals: function(Integer, bool),
        ...
    };

Here, after member unfolding, `Integer` has structural type `{equals=function(Integer,bool), ...}`, and `Equalable<Integer>` has structural type `{equals=function(Integer,bool)}`, so they are indeed matched.


## 7. Generics

### 7.1 Universal Types

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


## 8. Predefined Types and Variables

The predefined types and variables are available in module `std`.

### 8.1 Low-level Functions

Functions that operates directly on primitive types. These functions either cannot be expressed by other functions, or is much slower without native implmentation. All the names of functions start with '@'.

Function | Type | Note
--- | --- | ---
@addi | `function(int,int,int)`
@subi | `function(int,int,int)`
@muli | `function(int,int,int)`
@divi | `function(int,int,int)`
@modi | `function(int,int,int)`
@negi | `function(int,int)`
@addf | `function(float,float,float)`
@subf | `function(float,float,float)`
@mulf | `function(float,float,float)`
@divf | `function(float,float,float)`
@modf | `function(float,float,float)`
@negf | `function(float,float)`
@and | `function(int,int,int)`
@or | `function(int,int,int)`
@not | `function(int,int)`
@xor | `function(int,int,int)`
@gei | `function(int,int,int)`
@gti | `function(int,int,int)`
@lei | `function(int,int,int)`
@lti | `function(int,int,int)`
@eqi | `function(int,int,int)`
@nei | `function(int,int,int)`
@gef | `function(float,float,int)`
@gtf | `function(float,float,int)`
@lef | `function(float,float,int)`
@ltf | `function(float,float,int)`
@eqf | `function(float,float,int)`
@nef | `function(float,float,int)`
@ftoi | `function(float,int)`
@ctoi | `function(char,int)`
@itof | `function(int,float)`
@itoc | `function(int,char)`
@array | `function(int,array(char))` | Create an array, given length
@arrayi | `function(int,array(int))` |
@arrayf | `function(int, array(float))`
@aget | `function(array(char),int,char)`
@ageti | `function(array(int),int,int)`
@agetf | `function(array(float),int,float)`
@aset | `function(array(char),int,char,nil)`
@aseti | `function(array(int),int,int,nil)`
@asetf | `function(array(float),int,float,nil)`
@bool | `function(top,int)` | converting anything other than 0,0.0f,false to 1
@throw | `function(array(char), bottom)`
@exit | `function(nil)` | Exit
@alloc | `forall<X>.function(int,array(X))`
@set | `forall<X>.function(array(X),int,X,nil)`
@get | `forall<X>.function(list,int,X)` | Fetch and casting. This is the only weak type
@len | `function(Addressable,int)`
@copy | `function(Addressable,int,int,Addressable,int)` | Copy #2 from #0:#1 to #3:#4.
@open | `function(array(char),int,int)` | Open a file with mode #1, return file descriptor
@close | `function(int,nil)` | Close a file
@read | `function(int,int,char,array(char))` | Read #1 bytes from file descriptor #0; if #2 != 0 will stop when #2 is meet.
@write | `function(int,array(char),int)` | Write string to file descriptor #0
@format | `function(array(char), array(Addressable), array(char))`

### 9.2 Extended Functions

Arithemetic and basic functional programming:

Function | Type | Usage
--- | --- | ---
id | `forall<X>.function(X,X)` | identity
const | `forall<X,Y>.function(X,Y,X)` | return the first one
seq | `forall<X,Y>.function(X,Y,Y)` | return the second one
dot | `forall<X,Y,Z>.function(function(X,Y),` `function(Y,Z),function(X,Z))` | function product
flip |`forall<X,Y,Z>.function(function(X,Y,Z),` `function(Y,X,Z))` | flip the first and second argument
fst | `forall<X,Y>.function(tuple(X,Y),X)` | first tuple member
snd | `forall<X,Y>.function(tuple(X,Y),Y)` | second tuple member
curry | `forall<X,Y,Z>.function(function(X,Y,Z),` `function(X,function(Y,Z)))`
uncurry | `forall<X,Y,Z>.function(function(X,function(Y,Z)),` `function(X,Y,Z))`
until | `forall<X>.function(function(X,Bool),function(X,X),` `function(X,X))` | continuously apply function until cond(a) is true
error | `function(array(char),bottom)` | same as @error
undefined | `function(bottom)` | @error("undefined")
fix | `forall<X>.function(function(X,X),function(X,X))` | fixed-point combinator
sel | `forall<X>.function(Bool,X,X)` | selects first or second, depends on parameter
sand | `function(function(Bool),function(Bool),Bool)` | shortcut and
sor | `function(function(Bool),function(Bool),Bool)` | shortcut or

Arrays and lists:

Function | Type | Usage
--- | --- | ---
aget | `forall<X>.function(array(X),int,X)`
aset | `forall<X>.function(array(X),int,X,array(X))`
