# Mini: Feature Plan

Yes -- Has plan to support
No  -- No plan to support
LT  -- As a long term plan
TBD -- Not decided yet 

Version a.b: Supported before or within that version;
Version a+: Supported within that major version;

Compatibility is not guaranteed before 1.0.

### Roadmap

Version | Main Feature
--- | ---
0.1 | Basic expression system, function, binding, assignment, type system
0.2 | Generic types and interface
0.3 | Object system and recursive types
0.4 | Pattern matching and standard libraries
0.5 | Variants and pattern match on types
0.6 | Generic classes and interfaces
0.7 | Language revision and codegen optimization
0.8 | Garbage collection and runtime optimization

### Basics

Name | Availablity | Notes
--- | --- | ---
Compiled | 0.1 | Maybe To C or just VM object. Requires static type
Compile to executable | 0.3,Partially | Will be java classes-like executables
Interactive interpretation | LT |
Function as object, lambda | 0.1 |
Global variable | 0.1 |
Scope shadowing | 0.1 | 
Lambda captures local variable | 0.1 | 
Currying and partial evaluation | No | Requires lazy eval, or special 'bind' function
Lazy evaluation | No |
Lazy evaluation by default | No | Requires currying and nonsequential execution
Pattern matching by structured binding | 0.5 | 
Variable assignment | 0.1 | Requires sequential execution
Class member assignment | 0.3 | Requires sequential eval and (lvalue or overload). May achieved via function
Call by sharing | 0.1 | Requires no deep copy
Viable length function arguments | No |
Module importing | 0.1 | Without namespace
Namespace | 0.7 |
String literal | 0.1 |
Operators | No |
Macro | No |

### Type System

Name | Availablity | Notes
--- | --- | ---
Primitive atomic types | 0.1 | int, char, float, bool, nil
Primitive collection types | 0.1 | array, tuple, function
Duck type | 0.2 | struct
Static type inference | 0.1 | But maybe more dynamical in the future
Reference type | No |
Generic functions | 0.2 |
Generic classes | 0.6 |
Unknown/any/undefined/maybe | 0.5 |
Type constraint | 0.2 |
Existential types | TBD |
Full universal types | 0.2 | 
Recursive types | 0.3 |
Multual recursive types | 0.3 |
Automatic type deduction | No |
Runtime type info (RTTI) | LT | typeof() returns the corresponding type

### Object System

Name | Availablity | Notes
--- | --- | ---
Object system | 0.3 |
Object inheritance | 0.3 |
Multiple inheritances | No |
Constructor | 0.3 |
Retrieve base class constructor | 0.3
Multiple constructors | 0.3 |
Methods with self-reference | 0.3 |
Retrieve base class in methods | No |
Static members | No |
Initialization by duck typing | 0.3 |
Runtime dispatching | 0.3 | 
Interfaces | 0.2 |

### Control

Name | Availablity | Notes
--- | --- | ---
Pattern matching | 0.4,0.5 |
Short-circuit if/else | No | Redundant if pattern matching is supported
Loop/fixed combinator | 0.4,0.8 | Provided as function; tail recursion optimization
Continuation/Coroutine | No | Should be covered by lambdas
