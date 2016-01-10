#include "interpreter.hpp"
#include "numberparse.hpp"
#include <cassert>

namespace Lithium {

static const std::string get_keyword("get"),
    set_keyword("set"),
    call_keyword("call"),
    function_keyword("function"),
    if_keyword("if"),
    return_keyword("return"),
    up_keyword("up"),

    clone_keyword("clone"),

    int_keyword("int"),
    float_keyword("float"),
    boolean_keyword("bool"),
    string_keyword("string"),
    array_keyword("array"),
    prototype_keyword("prototype");

// A simple state machine that skips a scope.
// Accepts with ctx sitting on the ':', ends with ctx sitting on the '.'
static void skip_scope(Context &ctx){
    assert(ctx.source().peekc()==':');
    ctx.source().getc();
    ctx.source().skipWhitespace();

    bool in_string = false;
    uint64_t scope_level = 1;

    char c = ctx.source().peekc();
have_char:
    if(!ctx.source().valid())
        return;

    if(c=='"')
        in_string=!in_string;
    else if(in_string){
        if(c=='\\')
            ctx.source().getc();
    }
    else{
        if(c=='.'){
            if(!(--scope_level))
                return;
        }
        else if(c==':')
            scope_level++;
        else if(c=='%')
            while(ctx.source().valid() && ctx.source().peekc()!='\n')
                ctx.source().getc();
    }

    c = ctx.source().getc();
    goto have_char;
}

  //  <program>        ::= [<statement> '\n']*
bool InterpretProgram(Context &ctx){
    while(ctx.source().skipWhitespace() &&
        InterpretStatement(ctx)){
        ctx.source().skipWhitespace();
        if(ctx.source().peekc()!='\n')
            break;
    }
    return true;
}

  //  <statement>      ::= <set> | <call> | <variable_decl> | <function_decl> | <if> | <loop> | <return> | <up>
bool InterpretStatement(Context &ctx){
    Source start = ctx.source();

    std::string ident;
    if(!ctx.source().getAlphaIdentifier(ident))
        return false;

    if(ident==set_keyword)
        return InterpretSet(ctx);
    else if(ident==call_keyword)
        return InterpretCall(ctx);
    else if(ident==function_keyword)
        return InterpretFunctionDeclaration(ctx);
    else if(ident==if_keyword)
        return InterpretIf(ctx);
    else if(ident==return_keyword)
        return InterpretReturn(ctx);
    else if(ident==up_keyword)
        return InterpretUp(ctx);
    else{
        ctx.source() = start;
        return InterpretVariableDeclaration(ctx);
    }

}

  //  <set>            ::= 'set' <identifier> <expression> // NOTE <identifier> should really be the same as 'get' minus the keyword.
bool InterpretSet(Context &ctx){
    
    std::string name;
    if(!ctx.source().getIdentifier(name))
        return false;

    if(!InterpretExpression(ctx))
        return ctx.setError(Context::Error::SyntaxError, "Expected variable name for set");

    ctx.addVariable(name, ctx.top());
    ctx.pop();

    return true;
}

  // <call>           ::= 'call' <expression> '(' (<expression>, )* ')'
bool InterpretCall(Context &ctx){

    if(!InterpretExpression(ctx))
        return false;

    if(ctx.top().type!=Value::Function)
        return ctx.setError(Context::Error::TypeError, "Value is not a function");

    Value function = ctx.pop();

    ctx.source().skipWhitespace();

    if(!ctx.source().match('('))
        return ctx.setError(Context::Error::SyntaxError, "Expected start of argument list");

    std::vector<Value> args; args.reserve(function.value.function->args.size());

    for(const std::pair<std::string, TypeSpecifier> &i : function.value.function->args){
        ctx.source().skipWhitespace();

        if(!InterpretExpression(ctx))
            return false;

        if(ctx.top().type!=i.second.our_type)
            return ctx.setError(Context::Error::TypeError, 
                std::string("Argument") + std::to_string(args.size()) + " is a " + ValueName(ctx.top().type) + ", expected " + ValueName(i.second.our_type));

        args.push_back(ctx.pop());

        ctx.source().skipWhitespace();

        if(!ctx.source().match(','))
            return ctx.setError(Context::Error::SyntaxError, "Expected comma before next argument");
    }

    ctx.source().skipWhitespace();
    if(!ctx.source().match(')'))
            return ctx.setError(Context::Error::SyntaxError, "Expected close paren after argument");

    assert(args.size() == function.value.function->args.size());

    // Setup the new scope
    Value val;
    val.type = Value::Object;
    val.value.object = new std::map<std::string, Value>();

    for(uint64_t i = 0; i<args.size(); i++){
        val.value.object->insert({function.value.function->args[i].first, args[i]});
    }

    const uint64_t scope_num = ctx.scopes.size();

    ctx.scopes.push_back({ctx.source().position(), ctx.source().position(), val});
    ctx.source().position(function.value.function->start);

    if(!ctx.source().match(':'))
        return ctx.setError(Context::Error::SyntaxError, "Expected colon at start of function");

    if(!InterpretProgram(ctx))
        return false;

    if(!(ctx.source().skipWhitespace() && ctx.source().match('.')))
        return ctx.setError(Context::Error::SyntaxError, "Expected dot at close of function");

    if(scope_num!=ctx.scopes.size())
        return ctx.setError(Context::Error::SyntaxError, "Expected return statement in function");

    return true;
}
/*
bool InterpretIf(Context &ctx){

    if(!InterpretExpression(ctx))
        return false;

    if(ctx.source().peekc()!=':')
        return ctx.setError(Context::Error::SyntaxError, "Expected start of expression at end of if statement");

    bool success = false;

    switch(ctx.top().type){
        case Value::Null:
            return ctx.setError(Context::Error::ReferenceError, "(INTERNEL) Null reference");
        case Value::Floating:
            success = ctx.top().value.floating; break;
        case Value::Integer:
            success = ctx.top().value.integer; break;
        case Value::Boolean:
            success = ctx.top().value.boolean; break;
        default:
            return ctx.setError(Context::Error::TypeError, 
                std::string("Expression in if statement is a ") + ValueName(ctx.top().type) + ", expected Integer, Floating, or Boolean");
    }

    const uint64_t if_line = ctx.source().line();

    if(success){
        assert(ctx.source().peekc()==':');
        ctx.source().getc();
        if(!InterpretProgram(ctx))
            return false;
        ctx.source().skipWhitespace();

        if(!ctx.source().match('.'))
            return ctx.setError(Context::Error::SyntaxError, "Expected end of scope for if statement");
        else
            return true;
    }
    else{
        skip_scope(ctx);
    }
    if(!ctx.source().match('.'))
        return ctx.setError(Context::Error::SyntaxError, std::string("Expected end of scope after if statement on line ") + std::to_string(if_line));
    else
        return true;
}
*/

bool InterpretIf(Context &ctx){
    const uint64_t if_line = ctx.source().line();
    if(!InterpretExpression(ctx))
        return false;
    if(!ConditionalType(ctx))
        return false;
    if(!ConditionalSuccess(ctx.pop())){
        skip_scope(ctx);
        return true;
    }
    else if(!InterpretScope(ctx))
        return false;
    else if(!ctx.source().match('.'))
        return ctx.setError(Context::Error::SyntaxError, std::string("Expected end of scope after if statement on line ") + std::to_string(if_line));

    return true;
}

// TODO
bool InterpretLoop(Context &ctx){

    Source start = ctx.source();
    do{
        if(!InterpretExpression(ctx))
            return false;
        if(!ConditionalType(ctx))
            return false;
        if(!ConditionalSuccess(ctx.pop())){
            skip_scope(ctx);
            break;
        }
        else{
            if(!InterpretScope(ctx))
                return false;
            ctx.source() = start;
        }
    }while(true);

    if(!ctx.source().match('.'))
        return ctx.setError(Context::Error::SyntaxError, std::string("Expected end of scope after loop on line ") + std::to_string(start.line()));

    return true;
}

bool InterpretReturn(Context &ctx){

    if(!InterpretExpression(ctx))
        return false;
    ctx.source().position(ctx.scopes.back().end);
    ctx.scopes.pop_back();

    return true;
}

bool InterpretUp(Context &ctx){

    ctx.source().position(ctx.scopes.back().end);
    ctx.scopes.pop_back();

    return true;
}

bool InterpretExpression(Context &ctx){
    if(!InterpretTerm(ctx))
        return false;

    while(ctx.source().skipWhitespace()){
        const char c = ctx.source().peekc();
        if(c=='+' || c=='-'){
            ctx.source().getc();
            ctx.source().skipWhitespace();

            Value first = ctx.pop();
            if(!InterpretTerm(ctx))
                return false;
            Value second = ctx.pop();

            Value::Type mutual_cast = MutualCast(first, second);
            bool is_arith = TypeIsArithmetic(mutual_cast);
            if(!is_arith)
                return ctx.setError( Context::Error::TypeError, std::string("Types ") + ValueName(first.type) + " and " + ValueName(first.type) + " are not valid for arithmetic");

            MutualCastValue(first, second, mutual_cast);

            if(c=='+'){
                ValueBinaryOpIntegerOrFloating<std::plus>(first, second);
            }
            else if(c=='-'){
                ValueBinaryOpIntegerOrFloating<std::minus>(first, second);
            }
            ctx.push(first);

        }
        else
            break;
    }
    return true;
}

bool InterpretTerm(Context &ctx){
    if(!InterpretFactor(ctx))
        return false;

    while(ctx.source().skipWhitespace()){
        const char c = ctx.source().peekc();
        if(c=='*' || c=='/'){
            ctx.source().getc();
            ctx.source().skipWhitespace();

            Value first = ctx.pop();
            if(!InterpretFactor(ctx))
                return false;
            Value second = ctx.pop();

            Value::Type mutual_cast = MutualCast(first, second);
            bool is_arith = TypeIsArithmetic(mutual_cast);
            if(!is_arith)
                return ctx.setError( Context::Error::TypeError, std::string("Types ") + ValueName(first.type) + " and " + ValueName(first.type) + " are not valid for arithmetic");

            MutualCastValue(first, second, mutual_cast);

            if(c=='*'){
                ValueBinaryOpIntegerOrFloating<std::multiplies>(first, second);
            }
            else if(c=='/'){
                ValueBinaryOpIntegerOrFloating<std::divides>(first, second);
            }
            ctx.push(first);

        }
        else
            break;
    }
    return true;
}

enum e_bitop { z, shr, shl, ror, rol, bor, band, bxor };

inline bool is_double_char_bitop(e_bitop b){
    return b==bor || b==band || b==bxor;
}

inline e_bitop is_bitop(char c1, char c2){
    if(c1!=c2){
        if(c1=='|' && c2=='>')
            return ror;
        if(c1=='<' && c2=='|')
            return rol;
        if(c1=='|')
            return bor;
        if(c1=='&')
            return band;
        if(c1=='^')
            return bxor;
    }
    else{
        if(c1=='>')
            return shr;
        if(c1=='<')
            return shl;
    }
    return z;
}

bool InterpretFactor(Context &ctx){
    if(!InterpretValue(ctx))
        return false;

    while(ctx.source().skipWhitespace()){
        Source start = ctx.source();
        const char c1 = start.getc();
        const char c2 = start.getc();

        if(!start.valid())
            return ctx.setError( Context::Error::SyntaxError, "Unexpected end of input" );
        if(e_bitop e = is_bitop(c1, c2)){
            ctx.source().getc();
            if(is_double_char_bitop(e))
                ctx.source().getc();
            ctx.source().skipWhitespace();

            Value first = ctx.pop();
            if(!InterpretValue(ctx))
                return false;
            Value second = ctx.pop();

            Value::Type mutual_cast = MutualCast(first, second);
            bool is_arith = TypeIsArithmetic(mutual_cast);
            if(!is_arith)
                return ctx.setError( Context::Error::TypeError, std::string("Types ") + ValueName(first.type) + " and " + ValueName(first.type) + " are not valid for arithmetic");

            MutualCastValue(first, second, mutual_cast);

            if(e==band){
                ValueBinaryOpInteger<std::bit_and>(first, second);
            }
            else if(e==bor){
                ValueBinaryOpInteger<std::bit_or>(first, second);
            }
            else if(e==bxor){
                ValueBinaryOpInteger<std::bit_xor>(first, second);
            }
            else if(e==shr){
                ValueBinaryOpInteger<arith::bitshiftright>(first, second);
            }
            else if(e==shl){
                ValueBinaryOpInteger<arith::bitshiftleft>(first, second);
            }
            else if(e==ror){
                ValueBinaryOpInteger<arith::bitrotateright>(first, second);
            }
            else if(e==rol){
                ValueBinaryOpInteger<arith::bitrotateleft>(first, second);
            }

            ctx.push(first);

        }
        else
            break;
    }
    return true;
}

bool InterpretValue(Context &ctx){
    ctx.source().skipWhitespace();

    const char c = ctx.source().peekc();

    if(c=='g' || c=='c'){

        std::string val;
        ctx.source().getAlphaIdentifier(val);

        if(val==get_keyword)
            return InterpretGet(ctx);
        else if(val==call_keyword)
            return InterpretCall(ctx);
        else if(val==clone_keyword)
            return InterpretObjectLiteral(ctx);
        else
            return ctx.setError( Context::Error::SyntaxError, std::string("Expected value as get, call, literal, or ( <expression> ) at ") + val );
    }
    else if(Source::isNum(c)){
        return InterpretNumberLiteral(ctx);
    }
    else if(c=='"'){
        return InterpretStringLiteral(ctx);
    }
    else if(c=='~' || c=='`'){
        return InterpretBooleanLiteral(ctx);
    }
    else if(c=='['){
        return InterpretArrayLiteral(ctx);
    }
    else if(c=='{'){
        return InterpretObjectLiteral(ctx);
    }
    else if(c=='('){
        ctx.source().getc();
        ctx.source().skipWhitespace();
        if(!InterpretExpression(ctx))
            return false;
        ctx.source().skipWhitespace();
        if(!ctx.source().match(')'))
            return ctx.setError( Context::Error::SyntaxError, "Expected close paren after nested expression" );
        else
            return true;
    }
    else
        return ctx.setError( Context::Error::SyntaxError, "Expected value as get, call, literal, or ( <expression> )");

}

 // <get>            ::= 'get' <variable> [ '[' <type> <expression> ']' ]
bool InterpretGet(Context &ctx){

    ctx.source().skipWhitespace();
    std::string ident;
    if(!ctx.source().getIdentifier(ident))
        return ctx.setError( Context::Error::SyntaxError, "Expected identifier after type in get statement" );

    Value val = ctx.findObject(ident);
    if(val.type==Value::Null)
        return ctx.setError( Context::Error::ReferenceError, std::string("Reference to undefined variable ") + ident );
    else if(val.type!=Value::Object && val.type!=Value::Object && val.type!=Value::Object)
        return ctx.setError( Context::Error::TypeError, std::string("Cannot fetch from a ") + ValueName(val.type) );

    ctx.source().skipWhitespace();

    if(ctx.source().peekc()=='['){
        ctx.source().getc();

        ctx.source().skipWhitespace();
        TypeSpecifier type;
        // TODO: Make typing strict here.
        if(!InterpretType(ctx, type))
            return ctx.setError( Context::Error::SyntaxError, "Expected type specifier for get statement" );

        // This check is an extra early out.
        if(val.type==Value::Array && !val.value.array->empty())
            if(val.value.array->back().type!=type.our_type)
                return ctx.setError( Context::Error::TypeError, std::string("Invalid fetch of type ") + ValueName(type.our_type) +
                " from Array holding type " + ValueName(val.value.array->back().type) );

        ctx.source().skipWhitespace();

        if(!InterpretExpression(ctx))
            return false;

        Value index = ctx.pop();
        Value fetch = { Value::Null, {}};

        switch(val.type){
            case Value::Array:
                if(index.type!=Value::Integer)
                    return ctx.setError( Context::Error::TypeError, std::string("Cannot access element of an array from a ") + ValueName(index.type) );
                if(index.value.integer < 0)
                    return ctx.setError( Context::Error::ReferenceError, std::to_string(index.value.integer) + " is negative in Array fetch" );
                if((uint64_t)index.value.integer >= val.value.array->size())
                    return ctx.setError( Context::Error::ReferenceError, std::to_string(index.value.integer) + 
                        " is past end of array of size " + std::to_string(val.value.array->size()) );

                fetch = val.value.array->at(index.value.integer);
                break;
            case Value::String:
                if(index.type!=Value::Integer)
                    return ctx.setError( Context::Error::TypeError, std::string("Cannot access element of a string from a ") + ValueName(index.type) );
                if(index.value.integer < 0)
                    return ctx.setError( Context::Error::ReferenceError, std::to_string(index.value.integer) + " is negative in String fetch" );
                if((uint64_t)index.value.integer >= val.value.string->length())
                    return ctx.setError( Context::Error::ReferenceError, std::to_string(index.value.integer) + 
                        " is past end of array of size " + std::to_string(val.value.string->length()) );
                fetch.type = Value::Integer;
                fetch.value.integer = val.value.string->at(index.value.integer);
                break;
            case Value::Object:
                if(index.type!=Value::String)
                    return ctx.setError( Context::Error::TypeError, std::string("Cannot access element of an object from a ") + ValueName(index.type) );

                {
                    auto x =  val.value.object->find(index.value.string[0]);
                    if(x == val.value.object->cend())
                        return ctx.setError( Context::Error::ReferenceError, std::string("No such element '") + index.value.string[0] + '\'' );
                    else
                        fetch = x->second;
                }
                break;
            default:
                assert(false);
        }

        assert(fetch.type!=Value::Null);
        if(fetch.type==Value::Null)
            return ctx.setError( Context::Error::ReferenceError, "(INTERNAL) Fetch failed" );

        // TODO: Make typing strict here.
        if(fetch.type!=type.our_type)
            return ctx.setError( Context::Error::TypeError, ident + " is type " + ValueName(val.type) + " but was accessed as type " + ValueName(type.our_type) );

        ctx.push(fetch);

        ctx.source().skipWhitespace();

        if(!ctx.source().match(']'))
            return ctx.setError( Context::Error::TypeError, "Expected close backet at the end of access" );
        return true;
    }
    else{

        ctx.push(val);
        return true;
    }

}

bool InterpretStringLiteral(Context &ctx){

    ctx.source().skipWhitespace();
    std::string *str = new std::string();
    if(!ctx.source().getStringLiteral(str[0])){
        delete str;
        return ctx.setError( Context::Error::SyntaxError, "Expected string literal.");
    }

    ctx.source().skipWhitespace();

    Value val; val.type = Value::String; val.value.string = str;
    ctx.push(val);
    return true;
}

bool InterpretNumberLiteral(Context &ctx){
    Value val;
    ctx.source().skipWhitespace();
    if(!ParseNumberLiteral(ctx, val))
        return ctx.setError( Context::Error::SyntaxError, "Expected number literal" );
    ctx.push(val);
    return true;
}

bool InterpretBooleanLiteral(Context &ctx){
    ctx.source().skipWhitespace();
    const char c = ctx.source().getc();
    Value val; val.type = Value::Boolean;
    if(c=='`'){
        val.value.boolean = true;
    }
    else if(c=='~'){
        val.value.boolean = false;
    }
    else
        return ctx.setError( Context::Error::SyntaxError, "Expected boolean literal (` or ~)" );

    ctx.push(val);
    return true;
}

bool InterpretArrayLiteral(Context &ctx){
    ctx.source().skipWhitespace();
    if(ctx.source().peekc()=='{'){



    }
    else{
        std::string prototype;
        ctx.source().getIdentifier(prototype);

    }
        return ctx.setError( Context::Error::SyntaxError, "Expected array literal" );

    std::unique_ptr<std::vector<Value>> array(new std::vector<Value>());

    ctx.source().skipWhitespace();

    if(!InterpretExpression(ctx))
        return false;

    array->push_back(ctx.pop());
    const Value::Type must_be = array->back().type;

    while(ctx.source().peekc()==','){
        ctx.source().skipWhitespace();

        if(!InterpretExpression(ctx))
            return false;

        if(must_be!=ctx.top().type)
            return ctx.setError( Context::Error::TypeError, std::string("Invalid element of type ") + ValueName(ctx.top().type) + ", expected " + ValueName(must_be) );

        array->push_back(ctx.pop());

        ctx.source().skipWhitespace();
    }

    if(!ctx.source().match(']'))
        return ctx.setError( Context::Error::SyntaxError, "Expected comma or close bracket after element of array" );

    Value val;
    val.type = Value::Array;
    val.value.array = array.release();

    ctx.push(val);

    return true;

}

bool InterpretObjectLiteral(Context &ctx){
    ctx.source().skipWhitespace();
    if(!ctx.source().match('['))
        return ctx.setError( Context::Error::SyntaxError, "Expected array literal" );

    std::unique_ptr<std::vector<Value>> array(new std::vector<Value>());

    ctx.source().skipWhitespace();

    if(!InterpretExpression(ctx))
        return false;

    array->push_back(ctx.pop());
    const Value::Type must_be = array->back().type;

    while(ctx.source().peekc()==','){
        ctx.source().skipWhitespace();

        if(!InterpretExpression(ctx))
            return false;

        if(must_be!=ctx.top().type)
            return ctx.setError( Context::Error::TypeError, std::string("Invalid element of type ") + ValueName(ctx.top().type) + ", expected " + ValueName(must_be) );

        array->push_back(ctx.pop());

        ctx.source().skipWhitespace();
    }

    if(!ctx.source().match(']'))
        return ctx.setError( Context::Error::SyntaxError, "Expected comma or close bracket after element of array" );

    Value val;
    val.type = Value::Array;
    val.value.array = array.release();

    ctx.push(val);

    return true;

}

bool InterpretVariableDeclaration(Context &ctx){
    TypeSpecifier type;

    ctx.source().skipWhitespace();

    if(!InterpretType(ctx, type))
        return ctx.setError( Context::Error::SyntaxError, "Expected type specifier" );

    if(!VerifyPrototypes(ctx, type))
        return ctx.setError( Context::Error::ReferenceError, "Unknown prototype" );

    ctx.source().skipWhitespace();

    std::string name;
    if(!ctx.source().getIdentifier(name))
        return ctx.setError( Context::Error::SyntaxError, "Expected variable name" );

    ctx.source().skipWhitespace();

    if(!InterpretExpression(ctx))
        return false;

    Value that = ctx.pop();

    if(that.type!=type.our_type)
        return ctx.setError( Context::Error::TypeError, name + " is of type " + ValueName(type.our_type) + " but is initialized with value of type " + ValueName(that.type) );

    ctx.addVariable(name, that);
    return true;

}

bool InterpretFunctionDeclaration(Context &ctx){
    ctx.source().skipWhitespace();

    std::string func_name;
    ctx.source().getIdentifier(func_name);

    ctx.source().skipWhitespace();

    if(!ctx.source().match('('))
        return ctx.setError( Context::Error::SyntaxError, "Expected start of argument list in function declaration" );

    Function func;
    

    do{
        ctx.source().skipWhitespace();

        TypeSpecifier type;
        if(!InterpretType(ctx, type))
            return ctx.setError( Context::Error::SyntaxError, "Expected type specifier" );

        std::string name;
        if(!ctx.source().getIdentifier(name))
            return ctx.setError( Context::Error::SyntaxError, "Expected argument name" );

        func.args.push_back({name, type});

        ctx.source().skipWhitespace();

    }while(ctx.source().peekc()==',');

    if(!ctx.source().match(')'))
        return ctx.setError( Context::Error::SyntaxError, "Expected close paren at end of argument list");

    ctx.source().skipWhitespace();
    if(ctx.source().peekc()!=':')
        return ctx.setError( Context::Error::SyntaxError, "Expected colon at start of function");

    func.start = ctx.source().position();

    skip_scope(ctx);

    return true;
}

bool InterpretType(Context &ctx, Value::Type &type){
    ctx.source().skipWhitespace();

    std::string type_str;
    ctx.source().getAlphaIdentifier(type_str);

    if(type_str==int_keyword)
        type = Value::Integer;
    else if(type_str==float_keyword)
        type = Value::Floating;
    else if(type_str==string_keyword)
        type = Value::String;
    else if(type_str==boolean_keyword)
        type = Value::Boolean;
    else if(type_str==array_keyword)
        type = Value::Array;
    else if(type_str==prototype_keyword)
        type = Value::Object;
    else if(type_str==function_keyword)
        type = Value::Function;
    else
        return false;

    return true;

}

bool InterpretType(Context &ctx, TypeSpecifier &type){
    Value::Type l_type;
    if(!InterpretType(ctx, l_type))
        return false;

    ctx.source().skipWhitespace();

    type.our_type = l_type;
    type.return_type = Value::Null;
    type.arg_types.clear();
    type.prototype.clear();

    switch(l_type){
        case Value::Null:
            return false;
        case Value::Integer: case Value::Floating: case Value::String: case Value::Boolean:
            return true;
        case Value::Array:
            if(!InterpretType(ctx, l_type))
                return false;
            type.return_type = l_type;
            return true;
        case Value::Object:
            return ctx.source().getIdentifier(type.prototype);
        case Value::Function:
            // TODO: actually parse functions.
            return false;
    }
    return false;
}

bool InterpretScope(Context &ctx){
    if(!ctx.source().match(':'))
        return ctx.setError(Context::Error::SyntaxError, "Expected colon at start of scope");

    if(!InterpretProgram(ctx))
        return false;

    if(ctx.source().peekc()!=':')
        return ctx.setError(Context::Error::SyntaxError, "Expected colon at start of scope");
    return true;
}

bool ConditionalType(Context &ctx){
    switch(ctx.top().type){
        case Value::Null:
            return ctx.setError(Context::Error::ReferenceError, "(INTERNEL) Null reference");
        case Value::Floating:
        case Value::Integer:
        case Value::Boolean:
            return true;
        default:
            return ctx.setError(Context::Error::TypeError, 
                std::string("Expression in if statement is a ") + ValueName(ctx.top().type) + ", expected Integer, Floating, or Boolean");
    }
}
bool ConditionalSuccess(Value val){
    switch(val.type){
        case Value::Floating:
            return val.value.floating;
        case Value::Integer:
            return val.value.integer;
        case Value::Boolean:
            return val.value.boolean;
        default:
            return false;
    }
}

} // namespace Lithium;


