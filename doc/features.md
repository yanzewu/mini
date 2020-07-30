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
0.2 | Basic object system, inheritance
0.3 | Advanced object system, member assignment, interface
0.4 | Monads, general semantic improvement, list system (not full Lisp)
0.5 | Basic generics (functions and classes)
0.6 | Advanced generics (special types, type-compatible monads, IO)
0.7 | Type constraints
0.8 | Expression object, short-circuit if
0.9 | Advanced control system

### Basics

Name | Availablity | Notes
--- | --- | ---
Compiled | 0.1 | Maybe To C or just VM object. Requires static type
Compile to executable | TBD |
Interactive interpretation | LT |
Function as object, lambda | 0.1 |
Global variable | 0.1 |
Scope shadowing | 0.1 | 
Lambda captures local variable | 0.1 | 
Currying and partial evaluation | TBD | Requires lazy eval, or special 'bind' function
Lazy evaluation | 0.5 |
Lazy evaluation by default | No | Requires currying and nonsequential execution
Pattern matching by structured binding | LT | Can be achieved by list manipulation
Variable assignment | 0.1 | Requires sequential execution
Class member assignment | 0.3 | Requires sequential eval and (lvalue or overload). May achieved via function
Call by sharing | 0.1 | Requires no deep copy
Viable length function arguments | No |
Module importing | 0.1 | Without namespace
Namespace | LT |
String literal | 0.1 |
Operators | TBD |
Macro | TBD |

### Type System

Name | Availablity | Notes
--- | --- | ---
Primitive atomic types | 0.1 | int, char, float, bool, nil
Primitive collection types | 0.1 | array, tuple, function
Duck type | 0.2 | struct
Static type inference | 0.1 | But maybe more dynamical in the future
Reference type | No |
Monads | 0.4 |
Generic functions and classes | 0.5 |
Unknown/any/undefined/maybe | 0.6 |
Type constraint | 0.7 |
Existential types | 0.7 |
Full Rank-2 types | TBD | 
Recursive types | TBD |
Multual recursive types | TBD |
Automatic type deduction | LT |
Runtime type info (RTTI) | LT | typeof() returns the corresponding type

### Object System

Name | Availablity | Notes
--- | --- | ---
Object system | 0.2 |
Object inheritance | 0.2 |
Multiple inheritances | No |
Constructor | 0.2 |
Retrieve base class constructor | 0.2
Multiple constructors | 0.2 |
Methods with self-reference | 0.3 |
Retrieve base class in methods | No |
Static members | No |
Initialization by duck typing | 0.2 |
Runtime dispatching | 0.3 | 
Interface | 0.3 |

### Control

Name | Availablity | Notes
--- | --- | ---
Do binding | 0.4 |
Short-circuit if/else | 0.8 | Requires quote/continuation, or just special syntax
Case | 0.8 | Same as lazy if/else
Loop | TBD | May be provided as function, but tail recursion is favored
Break/continue in loop | TBD |
First-class continuation | TBD |


### Lisp System

Name | Availablity | Notes
--- | --- | ---
Lisp-style list | 0.4 | 
Basic list manipultation | 0.4 |
Quote | 0.8 | 
Eval | 0.9 |
Apply | 0.9 |