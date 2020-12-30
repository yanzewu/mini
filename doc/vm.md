# Mini: VM Model and Execution Details

## 1. VM Model and Basic Instructions

### 1.1 Memory Layout

The runtime memory is divided into the following parts:

- Constant Region
- Global Region (Heap)
- Stack
- Program Counter (PC)

Constant region may be accessed only via special commands indirectly, which stores

- Bytecodes, i.e. closure objects without binding;
- Class layouts and metadata;

Global region can be directly accessed, which stores

- Global variables and functions.
- Allocated objects (includes arrays, tuples, structs, classes, closure objects).

The address of global region always start at 1. Address 0 is always invalid.

Stack stores working data and local variables. A frame is pushed to the top of stack each time a function call is made. So typically it's like

Frame1 -- Frame2 -- Frame3 -- ...

Each frame consists of

- Working stack: Variables used in each statement and before function call. Can be indexed from top. In the text below we use #1, #2, ... to represent the first, second, ... element from stack top.
- Local variable region: Can be directly fetched/stored. The size of local variable region is specified at compile-time. Arguments and binding variables are also mapped/copied to the local variable region.

Notice, in a practical implementation, the stack and local variable region may be continuous.

PC stores the current function (pointer to constant region) and index of bytecode. 


### 1.2 Basic Variable Storation and Addressing

Primitive data objects (nil, bool, char, int, float) are stored as raw value on the stack. `nil` and `bool` have same size as `char`. Functions, arrays and custom classes are stored as `address` on the stack (usually implemented as integer) to a certain location of memory. (Tuples and structs are to be determined).

#### 1.2.1  Addressing Local Variables

The `loadlocal`/`storelocal` bytecodes are used for loading/storing local variables:

    loadlocal  [index]
    loadlocali [index]
    loadlocalf [index]
    loadlocala [index]
    storelocal  [index]
    storelocali [index]
    storelocalf [index]
    storelocala [index]

For example, `loadlocal 3` loads the third local variable onto the stack top, therefore the stack size increases by 1. `storelocal 3` stores the stack top into the third local variable, replacing the existing value if necessary. The stack size decreases by 1.

The four version of suffix specify the bytes load/stored (for char, int, float and address), which is implementation specified. Usually the char version (with no suffix) loads 1 byte, while other versions load 4 bytes. 

#### 1.2.2 Addressing Global Variables

The `loadglobal`/`storeglobal` are used for loading/storing global variables:

    loadglobal   [globalindex]
    storeglobal  [globalindex]

Here the suffix is not necessary, since the type of global variable is predetermined.
Notice: `loadglobala` is used to load a variable which stores the address of a non-primitive object. Trying `loadglobal` on the allocated class directly will fail. Use `loadindex`/`loadfield` instead.

#### 1.2.3 Addressing Memory Objects with Offset

`loadglobal`/`storeglobal` reads the whole object only if it is a primitive type. But often we want a specific part of a non-primitive object. So there are two new instructions:

    loadindexx
    storeindexx

Here "x" represents either of empty, i, f, a. 

`loadindex` loads the value at an object with certain offset. The object address must be #2 stack top and the offset must be #1 stack top. The stack top will be replaced by the loaded value. For example, the following code gets the 5'th element from an int array with address 10:

    consta 10
    consti 5
    loadindexi

After calling loadindex, the two values for addressing are gone.

`storeindex` is the counterpart of `loadindex`, which stores the #1 stack top into the object reference at #3 stack top with offset at #2 stack top. For example, the following code stores 20 to the 5'th element to the global int array with address 10:

    consta 10
    consti 5
    consti 20
    storeindexi

#### 1.2.4 Addressing the Constant Pool

`loadconst` loads a value from constant pool. A corresponding object will be created in the heap and the address will be returned on the stack top. It is typically used for strings.

    loadconst [index]

#### 1.2.5 Allocate Object in Heap

`alloc` allocates a new continuous segment of memory in the heap with size specified at #1 stack top and returns the reference. The memory is not initialized.

    allocx

For example, if the stack top is 10, and we call `alloc`, then a continous segment of 10 char will be allocated in the heap. Assuming its address is 123, the 10 on stack top will be replaced by 123.


### 1.3 Storage and Addressing for Classes

Addressing class members are basically same as `loadindex/storeindex`; however we must know the offset of the member before addressing it. The offsets are stored in class layout table, which is a special object in global region. The table looks like

    [size of fields]
    field1: offset1
    field2: offset2
    ....

#### 1.3.1 Addressing Class Members

Therefore, the procedure to load a specific member from a certain instance is
1. Get the class layout table;
2. Look up the offset for the field we are interested in;
3. Get the address of instance;
4. Call `loadindex`;

Forturnately, there is a single bytecode doing all of these stuff:

    loadfield [fieldindex]
    storefield [fieldindex]

`loadfield` requires the object reference to be on stack top. `storefield` requires object reference to be #2 stack top and data to be #1 stack top. Note the suffix is not necessary for `loadfield`, since the size of data can be inferred from layout table, which stores inside the class instance.

#### 1.3.2 Addressing for Special Fields

For interfaces, the field index need to be lookuped during runtime. In the initialization time, the VM scan all field name indices (see Section 3.3) and construct a hashmap from field name to the actual field index. 

To load/store a field directly from field name index, use 

    loadinterface [fieldname]
    storeinterface [fieldname]

The field name is the index of field name in the constant pool. The stack change of `load/storeinterface` is similar as `load/storefield`.

#### 1.3.3 Allocation for Classes

Similar as addressing class members, if we want allocate memory for a new class, we must read the layout table to determine the size. The bytecode `new` does this altogether:

    new [index]

The new address will be pushed to stack top.

### 1.4 Functions

There are three types of functions in Mini:

- User functions: This is the normal functions defined by user, including closures with no binding variable or unbinded. Stores as list of bytecodes. Stores in constant region.
- Binded closure: Stores the binding variables and the address of the underlying user function. It's more like a class. Stores in heap/stack.
- Native functions: This is the preserved instructions for extension. Each native function has a unique id. Their behaviors are like function calls.

To invoke a user function or native function directly:

    call [constaddress]
    callnative [id]

Before calling, the arguments must be on stack top by declaration order (bottom-up). The result is returned to stack top.

To invoke a closure:

    calla

From bottom to top, the stack is first the address of closure, then arguments. The result is returned to stack top.

#### 1.4.1 Creating Closures

`newclosure` creates a closure with binding variables filled:

    newclosure [constaddress]

The binding variables must be on the stack top by declaration order. The address of newly-created closure is on the stack top.

#### 1.4.2 Other Stack Operations

Create a constant at stack top:

    const   [char literal]
    consti  [integer literal]
    constf  [float literal]
    consta  [address as integer]

Duplicate the stack top:

    dupx

Remove the stack top:

    pop

Swap the top two values:

    swap

#### 1.4.3 Exception

To stop the machine, use 

    halt

To throw an exception, use

    throw

where the stack top must be an object address. There is no restrict on how to handle the exception by the VM -- it may be displayed, or simply disgarded. However the machine is guaranteed to stop.


## 2. Value Instructions

### 2.1 Arithmatic and Logical Operations

Binary operations takes lhs = #2, rhs = #1 stack top. Unary operation (`not`) takes #1 stack top. 

Oprands of arithematic operations may only be `int` or `float`:

    addi addf
    subi subf
    muli mulf
    divi divf
    remi remf
    negi negf

 Oprands of logical operations may only be `int`.

    and
    or
    xor
    not

### 2.2 Comparison

Comparison is done by `cmp`, which takes lhs = #2, rhs = #1 stack top and returns integer -1 if lhs < rhs, 1 if rhs > lhs and 0 if lhs == rhs.

    cmp cmpi cmpf cmpa

The return of `cmp` can be further intepreted by a series of comparison codes:

    eq ne lt le gt ge

They take #1 stack top, if it meets the requirement then place _integer_ 1 on stack top; Otherwise place 0 on stack top.


### 2.3 Casts

Peform casts for #1 stack top.

    c2i     (char -> int)
    c2f
    i2f
    i2c
    f2i
    f2c


## 3. Implementation Detail

Here is some implementation details of Mini VM. The actual program implementation may differ from what is described here.

### 3.1 Constant Pool and VM Initialization

When the VM is started, it loads code and class definition table into the constant pool. The only objects in constant pool are

1. Functions: which stores the bytecode, size of local variable region, binding variable region and argument region for referring in function call or lambda creation. The structure is

        struct Function {
            ByteCode* bytecode;
            unsigned sz_code;
            unsigned sz_arg;
            unsigned sz_bind;
            unsigned sz_local;
            unsigned info_index;
        };

2. Class Layout Table: which stores the offset of each field. The structure is

        struct ClassLayout {
            unsigned* field_offset;
            unsigned sz;
            unsigned info_index;
        };

3. String: which is just a char array:

        struct StringConstant {
            char* value;
            unsigned sz;
        };

The global variables store in a special class whose layout table is always the first (with index 0). When the VM is initialized, the "global class" is automatically allocated.

Also, all initialization codes (including static varibles) in global region are put into a special function "\<main>" which is always the first function object. The function is automatically executed.

### 3.2 Heap Memory Objects

Anything in heap memory is stored as memory objects. The basic structure of a memory object is

    struct MemoryObject {
        Type type;
        unsigned int size;
        GCMetaData gc_metadata;
        void* data;
    };

The `type` of memory object may be one of 

    ARRAY
    CLOSURE
    CLASS
    
The first three types may only appear in the constant region and may not be indexed for their components. The structures are defined as

For arrays, the pointer just points to a continous segment of memory with specified size.

For closure it's defined by

    0 - 4 bytes: The index of underlying function in the constant pool;
    4 - size bytes: Binding variables, each consists of 4 bytes, same as in stack;

For a class it's defined by

    0 - 4 bytes: The constant pool index of the layout table.
    4 - size bytes: A continous segment of memory of variables, defined by layout table.

### 3.3 Debugging Information

Debugging information may be stored in the constant pool. Their indices are used by Function and ClassLayout objects. Typically, their structs are:

4. Function Info:

        struct FunctionInfo {
            unsigned name_index;
            unsigned* arg_type_index;
            unsigned ret_type_index;
            unsigned sz_arg;
        };

5. Class Info:

        struct ClassInfo {
            unsigned name_index;
            struct {
                unsigned field_name_index;
                unsigned field_type_index;
            }* field_info;
            unsigned sz_field;
        };

where all the indicies are pointing to string constants in the constant pool.

Finally, to address the specific line of program, the line information is stored in the line number table:

6. Line Number Table:

        struct LineNumbertable {
            struct {
                unsigned function_index;
                unsigned pc;
                unsigned line;
                }* lnt;
            unsigned sz_lnt;
        };

### Appdendix A: List of Instructions

Name | Argument | Stack Change | Note
-- | -- | -- | --
nop | 0 |  | No changes
halt | 0 | | Halt the machine
throw | 0 | address -> | Throw the exception
loadlocal(x) | 1:index | -> value | Load a local variable
loadindex(x) | 0 | address, index -> value | Load a value with certain offset
loadfield | 1:index | address -> value | Load a certain field from class instance
loadinterface | 1:fieldname | address -> value | Load an interface field from class instance
loadglobal | 1:index | -> value | Load a global variable
loadconst | 1:index | -> addr | Load a constant from constant pool (may allocate if necessary)
storelocal(x) | 1:index | value -> |
storeindex(x) | 0 | address,index,value -> |
storefield | 1:index | address,value -> |
storeinterface | 1:fieldname | address,value -> |
storeglobal | 1:index | value -> |
alloc(x) | 0 | size -> address | Allocate an array
new | 1:index of class | -> address | Create a class instance
newclosure | 1:index of function | binding1,binding2,... -> address | Create a closure instance
call | 1:index of function | arg1,arg2,... -> result | Call a function
calla | 1:count | function,arg1,arg2,... -> result | Call a closure with given address
callnative | 1:id | | Call a predefined function
ret(x) | 0 | value -> | Return a value and exit current function
retn | 0 | | Exit current function and shift the stack by 1
const(x) | 1:value | -> value | Create a constant to stack top
dup | 0 | value -> value,value | Duplicate the value at stack top
pop | 0 | value -> | Remove the value at stack top
swap | 0 | value1,value2 -> value2,value1 | Swap the top two values at stack top
shift | 0 | -> dummy | Stack grow by 1
add(x=i,f) | 0 | value1, value2 -> result |
sub(x) | 0 | value1, value2 -> result |
mul(x) | 0 | value1, value2 -> result |
div(x) | 0 | value1, value2 -> result |
mod(x) | 0 | value1, value2 -> result |
rem(x) | 0 | value1, value2 -> result |
and | 0 | value1, value2 -> result |
or| 0 | value1, value2 -> result |
xor | 0 | value1, value2 -> result |
not | 0 | value1 -> result |
cmp | 0 | value1, value2 -> result | Compare two char, return 0/1/-1
cmpi | 0 |value1, value2 -> result | Compare two int, return 0/1/-1
cmpf | 0 |value1, value2 -> result | Compare two float, return 0/1/-1
cmpa | 0 |value1, value2 -> result | Compare two addresses, return 0/1/-1
eq | 0 | value -> result | value == 0 ? 1:0 (int)
ne | 0 | value -> result |
gt | 0 | value -> result |
ge | 0 | value -> result |
lt | 0 | value -> result |
le | 0 | value -> result |
c2i | 0 | value -> result |
c2f | 0 | value -> result |
i2c | 0 | value -> result |
i2f | 0 | value -> result |
f2i | 0 | value -> result |
f2c | 0 | value -> result |


## Appendix B: Predefined Native Functions

ID | Name | Stack | Note
- | - | - | - |
0 | print | address of array(char) -> | Print a string to stdout.
1 | input | -> address of array(char) | Read a line from stdin, return the array address
2 | format | array(char), array(object) -> array(char) | Format an array of object in arg2 (will not read object value) with format given in arg1.
3 | len | object -> | Print the actual size of object.


