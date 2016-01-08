Lithium
=======

Copyright (c) 2015-2016, Martin McDonough

This is the third version of the Lithium Interpreter. It implements the LCL scripting language, which is the successor to the ICL language that Lithium used to implement.

What is LCL?
------------

LCL is a static and strongly typed, object-oriented language with a prototypical inheritance model.

LCL has a static type system with object-oriented features, supporting structured progamming, inheritence, a form of discriminated unions and 
Prolog-like rules, typed data accesses, and a simple grammar. It is superficially similar to Lisp, Smalltalk, Python, Io, and JavaScript, although 
it is most related to ICL (itself influenced heavily by Self and JavaScript) and Lisp.

A few unique features of LCL include:
 * All data accesses from objects are typed
 * A form of polymorphism is implemented by specifying a prototype for object accesses
 * Rules, similar to some Prolog predicates, are easily implemented using function typing
 * Functions can be overloaded based purely on return type
 * Highly unambiguous representation

A few examples:

Declaring variables
```
% Declaring some variables.
% The percent sign (%) marks comments.

% Declare a variable named x to be equal to 42.
int x 42

% Declare a variable named foo to be equal to the string "bar"
string foo "bar"

% Object literals use a syntax somewhat like JSON/JavaScript/Python
% bar contains an integer member named foo with a value of 1 and a string member named baz with a value of "gar"
% Just using 'object' rather than 'clone <prototype>' creates an object with the global prototype as its prototype
object bar { int foo 1, str baz "gar" }

```
Variable assignments
```
% Note that all variables must have an initial value assigned
int a 0
int b 1

% Assignments begin with the set keyword. This makes 'b' equal to 3
set b 3

% Variable accesses begin with the get keyword. This statement makes a equal to b.
set a get int b

% Create variable c, with the value a + b + 99, which is 3 + 3 + 99, so c equals 105.
int c get a + get b + 99

```
Accessing objects and arrays
```
object bar { int foo 1, string baz "gar" }

% Creats the variable x with the value 1, retrieved from the foo member of bar.
% Note that all fetches are typed, in this case int
int x get bar[int foo]

% All arrays have a type after the array keyword, marking their type. Arrays cannot contain dissimilar elements.
array int foo { 1, 2, 3, 4, 0, 42, 78 }

% y now contains the 5th 0-indexed element, here 42. It's coerced to a float in this example.
float y get foo[int 5]
```
