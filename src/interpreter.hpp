#pragma once
#include "context.hpp"
#include "variables.hpp"

namespace Lithium{

/*
    Quick, dirty, and only mostly correct description of the language.
    This reference will also apply to interpreter, since our bytecode is mostly just token code.

    <program>        ::= [<statement> '\n']*
    <statement>      ::= <set> | <call> | <variable_decl> | <function_decl> | <if> | <loop> | <return> | <up>
    <scope>          ::= ':' [<statement> '\n']* '.'
    <set>            ::= 'set' <identifier> <expression> // NOTE <identifier> should really be the same as 'get' minus the keyword.
    <call>           ::= 'call' <expression> '(' (<expression>, )* ')'

    <if>             ::= 'if' <expression> <scope>
    <loop>           ::= 'loop' <expression> <scope>
    <return>         ::= 'return' <expression>

    <expression>     ::= <term> [<addop> <term>]*
    <term>           ::= <factor> [<mulop> <factor>]*
    <factor>         ::= <value> [<bitop> <value>]*
    <value>          ::= <get> | <call> | <literal> | '(' <expression ')'
    <get>            ::= 'get' <variable> [ '[' <type> <expression> ']' ]
    <literal>        ::= <float_literal> | <int_literal> | <bool_literal> | <str_literal> | <arr_literal> | <obj_literal>

    <bool_literal>   ::= '`' | '~'
    <arr_literal>    ::= '[' <type> [ <expression> ','z ]* ']'
    <obj_literal>    ::= 'clone' <identifier> '{' (<type> <expression> ','z ) * '}'

    <variable_decl>  ::= <type> <identifier> <expression>
    <function_decl>  ::= 'function' <type> <identifier> '(' (<type> <identifier>','z )* ')' <scope>

    <type>           ::= ('float' | 'int' | 'bool' | 'string' | 'array' <type> | 'prototype' <identifier> | <function_type> )
    <function_type>  ::= 'function' <type> '(' (<type> ','z )* ')'
*/

bool InterpretProgram(Context &ctx);
inline bool Interpret(Context &ctx){ return InterpretProgram(ctx); }
bool InterpretStatement(Context &ctx);

bool InterpretSet(Context &ctx);
bool InterpretCall(Context &ctx);

bool InterpretIf(Context &ctx);
bool InterpretLoop(Context &ctx);
bool InterpretReturn(Context &ctx);
bool InterpretUp(Context &ctx);

bool InterpretExpression(Context &ctx);
bool InterpretTerm(Context &ctx);
bool InterpretFactor(Context &ctx);
bool InterpretValue(Context &ctx);
bool InterpretGet(Context &ctx);
bool InterpretStringLiteral(Context &ctx);
bool InterpretNumberLiteral(Context &ctx);
bool InterpretBooleanLiteral(Context &ctx);
bool InterpretArrayLiteral(Context &ctx);
bool InterpretObjectLiteral(Context &ctx);

bool InterpretVariableDeclaration(Context &ctx);
bool InterpretFunctionDeclaration(Context &ctx);

// Helpers...
bool InterpretType(Context &ctx, Value::Type &type);
bool InterpretType(Context &ctx, TypeSpecifier &type);
bool InterpretScope(Context &ctx);
bool GetConditional(Context &ctx);
bool ConditionalType(Context &ctx);
bool ConditionalSuccess(Value val);

} // namespace Lithium
