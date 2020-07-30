
# Mini: Language Specification

v0.1

## 1. Overview

### 1.1 Interpretation

Mini handles file as input. There is no plan for interactive mode in recent versions.

### 1.2 Statements

There are only two kinds of statements in Mini. One is "control statement", include declaration, definition, assignment, import, etc.. These statements begin with a single keyword, and are often separated with spaces:

    let a:int;
    set b = a;
    class Foo {x:int, y:int, new()->{}};    [future]

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

Intrinsic type deriving graph (! to be decided yet):

    object
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
              +---- type        [future]
              +---- expression  [future]
              +---- struct(f1=T1, f2=T2, ...)
              +---- all custom classes

                    +--- bottom [future]

All the atomic types cannot be inherited.

Each type (include `list`) corresponds to a literal.

- nil: `nil`, `()`
- bool: `true`/`false`
- int: integers
- float: float numbers
- char: `'a' 'b'`...
- tuple: `(a, b, ...)`
- function: closure
- type: all type names
- array: `[a, b, ...]`
- struct: `{x=a, y=b, ...}`

String literal is mapped to `array(char)`. To declare a integral float number, use format like "1.0".

### 2.2 Individual Types

#### Object

Object is the root type. It's for type hacking and type erasure.

#### Atomic

Instance of atomic cannot be created. It remains conceptual.

#### Nil

The empty type `nil` is for empty container and function returns.

#### List

`list` is not a generic type -- it has no type argument. It mainly serves as a type hack and future compatibility with Lisp.

#### Array

`array` is a generic type. It takes one type argument and hold objects with same type. So `array(int)` and `array(char)` are different types.

#### Tuple

`tuple` behaves like product type. It needs at least one argument:

    let a:tuple(int, int) = (1, 2);

#### Struct

`struct` is the recording type, which cannot be defined directly (but may be supported in the future). They can be created as instances:

    {a=1, b=2.0, c='3'}     // struct(a:int, b:float, c:char)

Named struct can be created through interfaces.

#### Function

`function` is the type for closure. All named/anoymous closures have type `function`, and can be created, assigned and passed just like other types. For a specific function, type

    function(T1, T2, ... Tn, Tret)

Represents a function accepting argument with type `T1`, ... , `Tn` by order and returns `Tret`.

#### Bottom

`bottom` is the child type of any types, and is reserved for error handling and compiling.

### 2.3 Variable Storation

All types deriving `list` are store by references. Currently, `atomic` types are stored by value, but this might be changed in the future.

### 2.4 Retriving Type Information [future]

`type` is a type for type objects. Function `typeof()` retrives a type object for variables and values:

    typeof(0);       # int
    typeof("123");   # array(char)
    
    def myadd(int a, int b)->int {add(a, b)};
    typeof(myadd);   # function(int,int,int)
    
    let foo:Foo;
    typeof(foo);     # Foo

`type` is also insensitive for what it has. All types `int`, `char`, even generic types (such as `array(int)`) are instances of `type`.

A bunch of conversion functions are provided for type conversion, which is listed in the end.

Type objects can be compared by `eq()`. Any different type is compared `false`. The ancester type is greater than a child type, hence they can be also compared by `le()`, `ge()`, `lt()`, `gt()`. Types not inheriting each other cannot be compared by these functions.



## 3. Variable System

### 3.1 Let Declaration

The syntax of variable definition is

    let [varname]:[type] (= initialize-expr);   // define a variable
    let [varname]:[type] (= structure);         // define a structure-initialized variable

The name of variable should not begin with 0-9 and reserved operators (like "()", "<>", "=") and must not coincide with reserved keywords. Variables starting with "@" will be treated as system symbols and thus also not allowed for declaration. It is strongly recommended to use letter at the beginning of variable and letter/number inside variable.

The `initialize-expr` is optional, and should yield an instance with the declared type or its child type. The new variable will have the declared type.

An exception for classes and interfaces is that `struct` instances can be used as the initialization expression (as the second case in the example). If the RHS is a structure, then (1) the declared type must be a class or interface; (2) all of RHS's fields must appear in the declared type.

The value of uninitialized variable is unspecified. The variable to be declared must not appear in the initialization expression. However, a variable with same name may be used if it is declared in the outer scope, and the newly declared variable will shadow the previous variable.

If a variable is defined globally, it has a global scope and can be used in any functions. If a variable is defined inside function, its access is limited in that function. A variable cannot be defined twice within the same scope.

Examples:

    let a:int;                # define a int
    let a:int = 2;            # define a int equals 2
    let b:int = add(a,1);     # define a int b equals a+1
    let l:list = (1,2,3);     # define a list

    class Foo {a:int, b:int};
    let foo1:Foo = {a=1, b=2};  # foo {a=1, b=2}
    let foo2:Foo = {b=2};  # foo {a=unspecified, b=2}


### 3.2 Variable Assignment

The syntax of assignment is

    set [varname] = [expr];
    set [varname].[field](.[field]...) = [expr];

Note the equal operator `=` only has syntactical meaning. The `varname` and `field` can ot be expressions.
More examples:

    let a:int;
    set a = 1;          # a = 1
    set a = add(a,2);   # a = 3
    set a = '1';        # Error: Type not match (int != str)

    class Foo {a:int, b:int};
    let foo1: Foo = {a=1, b=2}; # foo1 {a=1, b=2}
    set foo1.a = 2;             # foo1 {a=2, b=2}

Assignment of functions is also allowed, as long as their signature matches.

    set myadd = add;        # OK
    set myadd = opposite;   # Error: type not match (function(int,int,int) != function(int,int))

## 4. Functions

### 4.1 Closure Objects

A closure is represented by a capture list and a statement list (a single statement is treated as a list with one element). The general syntax is

    \(arg1:T1, arg2:T2, ..., argn:Tn)->(stat1, stat2, ..., statn):Tret
    \arg1:T1->stat:Tret
    \()->(stat1,...,statn):Tret
    
The capture list can either be a list of names or a single name. A function takes no argument has signature like `function(Tret)`.

The body of function is either a single statement, or a list of statements  separated by `,`. Function body cannot be empty. The last statement of function will be returned and hence it must be an expression. A function returns `nil` should put an expression that returns a `nil` in the end. (The monad semantics will be supported in the future).

Examples:

    let f:function(int, int, int) = \(a:int, b:int)->add(a, b):int;
    let gaussian:function(float, float, float) = (
        \(a:float, b:float)->(
            let y x*x, 
            exp(negative(y))
            ):float
        );

    let id:function(object, object) = \x:object->x:object;        # Identity
    let myadd:function(int, int, int) = add;     # Define a function f which equals add


### 4.2 Def Declaration

Alternatively, `def` provides a syntactic sugar for function definition:

    def bar (var1:type1, var2:type2)->type3  {FUNCTION BLOCK};

The statement above is equivalent to

    let bar:function(type1,type2,type3) \(var1:type1,var2:type2):type3->(FUNCTION BLOCK);

Define factorial:

    def factorial (x:int):int {
        if(ge(x, 1), x*factorial(x-1), 1)
    };

Function definition can be nested. The scope of defined function is same as the scope of its context - a global function can access global variables, and a local-defined function can access both local variables and variables in outer scope. However, a local function cannot change the value of binding variables (but can change fields!). `set` will raise error for such assignment.

### 4.3 Monads [future]



## 5. Object System [future]

### 5.1 Overview

An _object_ has two meanings: the type it represents (as in OOP) and the data it has (as structures). Mini supports both types of view. On one hand, types defined by `class` satisfies all features of type system; on the other hand, field polymorphism can be achieved via interface system.

### 5.2 Class Definition

Syntax:

    class CLASSNAME (extends SUPERCLASS) (implements INTERFACE) {
        DEFINITION BLOCK, ...
        CONSTRUCTOR, ...
    };

The definition block can contains variable definition or function definitions, separated by comma. Example:

    class Foo {
        a:int,
        b:char,
        let bar:function(int, int)
    };

### 5.3 Field Initialization

Classes may be initialized by structs. In this case, the constructor will not be called and the remain fields will be unspecified:

    let foo:Foo = {a=2};

By this feature, a class can be constructed by a certain function.

    class Point {x:int, y:int, get_r2:function(int)};
    def Point(x:int, y:int)->Point {
        let p:Point = {x=x, y=y},
        set p.get_r2 = \()->add(mul(p.x, p.x), mul(p.y, p.y)):int,
        p
    };


### 5.4 Constructor

The syntax for constructors are

    new(arg1:T1, ..., argn:Tn)->{FUNCTION BLOCK}
    new(arg1:T1, ..., argn:Tn) extends SUPERCLASS(EXPR1, EXPR2, ...)->{FUNCTION BLOCK}

The second type calls constructor of super class. By default it won't be called.

Keyword `self` and `super` will be avaiable inside the constructor function. If a class does not inherit others explicitly, `super` will be a dummy variable with type `object` (or `list` [future]).

If a class has constructor defined, then we can use `new` to create classes:

    class Point {
        x:int, y:int, get_r2:function(int),
        new(x:int, y:int)->{
            set self.x = x,
            set self.y = y,
            set self.get_r2 = \()->add(mul(self.x, self.x), mul(self.y, self.y)):int
        }
    };
    let p:Point = new Point(3, 4);
    p.get_r2();     // gives 25

(Though only `set` appears here, but other kinds of statements are also valid). A syntactic sugar is `defm` to define methods:

    class Point {
        x:int, y:int, get_r2:function(int),
        new(x:int, y:int)->{
            set self.x = x,
            set self.y = y,
            defm self.get_r2()->int {
                add(mul(self.x, self.x), mul(self.y, self.y))
            }
        }
    };

### 5.5 Inheritance

Class can be inherited with `extends`. A class may only have one base class.

    class Point {x:int, y:int};
    class ColoredPoint extends Point {color:tuple(float, float, float)};

The field names in the child classes cannot be same as nonstatic nonvirtual members in the base class. To inherit a specific member, use `virtual` to declare it in the super class:

    class Base {
        virtual print:function(nil),
        new()->{
            defm self.print()->{
                print("Base class\n");
            }
        }
    };
    class Derived extends Base {
        print:function(nil),    # not necessary here
        new()->{
            defm self.print()->{
                print("Derived class\n");
            }
        }
    }
    let b:Base = new Derived();
    b.print();  // "Derived class"

### 5.6 Static Members [future]

### 5.7 Interface [future]

Interfaces are aliases for certain `struct` types. Different interfaces equal to each other if their fields are the same.

The syntax of interface declaration is

    interface IFooable {
        foo: int
    };

An empty interface is equivalent to `object`.

Variables may have interface types:

    let foo:Fooable = {foo=2};

Interface member cannot be initialized. Interface can be (multiple) inherited, where the fields of super classes will be included.

    interface IBar extends IFooable {
        bar:int
    };

The type relationship between interfaces are only determined by their field, not by the explicit inheritance.

    interface IFooBar {
        foo:int,
        bar:int
    };
    let fb1:IBar;
    let fb2:IFooBar = {foo=3, bar=5};
    set fb1 = fb2;  // OK

To specify an interface for a class, use `implements`:

    class Bar implements Fooable {
        foo: int
    };

Similarly, the type relationship between interfaces are only determined by their field, not by the explicit implementation. However, if a class does not satisfy the `implements` in its declaration, the compilation will fail.
