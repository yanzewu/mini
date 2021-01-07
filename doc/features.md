# Mini: Feature Plan

Yes -- Supported
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
Compiled | Yes | Maybe To C or just VM object. Requires static type
Compile to executable | 0.7,Partially | Will be java classes-like executables
Interactive interpretation | LT |
Function as object, lambda | Yes |
Global variable | Yes |
Scope shadowing | Yes | 
Lambda captures local variable | Yes | 
Currying and partial evaluation | No | Requires lazy eval, or special 'bind' function
Lazy evaluation | No |
Lazy evaluation by default | No | Requires currying and nonsequential execution
Pattern matching by structured binding | 0.5 | 
Variable assignment | Yes | Requires sequential execution
Class member assignment | Yes | Requires sequential eval and (lvalue or overload). May achieved via function
Call by sharing | Yes | Requires no deep copy
Viable length function arguments | No |
Module importing | Yes | Without namespace
Namespace | 0.7 |
String literal | Yes |
Operators | No |
Macro | No |

### Type System

Name | Availablity | Notes
--- | --- | ---
Primitive atomic types | Yes | int, char, float, bool, nil
Primitive collection types | Yes | array, tuple, function
Duck type | Yes | struct
Static type inference | Yes | But maybe more dynamical in the future
Reference type | No |
Generic functions | Yes |
Generic classes | 0.6 |
Type constraint | Yes |
Existential types | TBD |
Full universal types | Yes | 
Recursive types | 0.6 |
Multual recursive types | 0.6 |
Automatic type deduction | No |
Runtime type info (RTTI) | 0.5 | typeof() returns the corresponding type

### Object System

Name | Availablity | Notes
--- | --- | ---
Object system | Yes |
Object inheritance | Yes |
Multiple inheritances | No |
Constructor | Yes |
Retrieve base class constructor | Yes
Multiple constructors | No |
Methods with self-reference | Yes |
Retrieve base class in methods | No |
Static members | No |
Initialization by duck typing | No |
Runtime dispatching | Yes | 
Interfaces | Yes |

### Control

Name | Availablity | Notes
--- | --- | ---
Pattern matching | 0.4,0.5 |
Short-circuit if/else | No | Redundant if pattern matching is supported
Loop/fixed combinator | Yes,0.7 | Provided as function; tail recursion optimization
Continuation/Coroutine | No | Should be covered by lambdas
