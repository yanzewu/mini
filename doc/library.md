
# Mini: Standard Library Functions

This is the list of bottom library functions.

Function:Type | Usage
- | -
alloc\<a>:function(int,array(a)) | Allocate an array
index\<a>:function(array(a),int,a) | Indexing an array
setindex\<a>:function(array(a),int,a,nil) | Set certain value of an array
first\<a,b>:function(tuple(a,b),a) | Get first of a pair
second\<a,b>:function(tuple(a,b),b) | Get second of a pair
eq\<Eq a>:function(a,a,bool) | Return two value equal
neq\<Eq a>:function(a,a,bool) | Return two value unequal
less\<Order a>:function(a,a,bool) | <
lesseq\<Order a>:function(a,a,bool) | <=
greater\<Order a>:function(a,a,bool) | >
greatereq\<Order a>:function(a,a,bool) | >=
int\<Num a>:function(a,int) | Cast a number to integer
float\<Num a>:function(a,float) | Cast a number to float
char\<Num a>:function(a,char) | Cast a number to char
bool\<a>:function(a, bool) | Cast an object to bool. Only {},nil,0,0.0,'\0' gives false
and:function(bool,bool,bool) |
or:function(bool,bool,bool) |
xor:function(bool,bool,bool) |
not:function(bool,bool) |
add:\<Num a>function(a,a,a) |
sub:\<Num a>function(a,a,a) |
mul:\<Num a>function(a,a,a) |
div:\<Num a>function(a,a,a) |
mod:\<Num a>function(a,a,a) |
pow:\<Num a>function(a,int,a) |
sqrt:function(float,float) |

