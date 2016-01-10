#pragma once
#include <vector>
#include <map>
#include <memory>
#include <cassert>

namespace Lithium{

struct Function;

struct Value{
    enum Type {
        Null, 
        Boolean,
        Integer,
        Floating,
        String,
        Object,
        Array,
        Function
    }type;

    union{
        int64_t integer;
        float floating;
        bool boolean;
        std::string *string;
        std::map<std::string, Value> *object;
        std::vector<Value> *array;
        struct Function *function;
    } value;

};

// A more complete set of type information than what is available from Value::Type.
struct TypeSpecifier{
    // our_type is the type of a variable.
    // return_type is the return type for functions and the array type for arrays.
    Value::Type our_type, return_type;
    // Prototype name for objects.
    std::string prototype;
    // arg_types is the type of the arguments.
    std::vector<TypeSpecifier> arg_types;
};

class Context;
bool VerifyPrototypes(Context &ctx, const TypeSpecifier &type);
class PrototypeVerifier{
    bool op(const TypeSpecifier &type);
public:
    Context &ctx;
    bool operator() (const TypeSpecifier &type){ return op(type); }
    bool operator() (const std::pair<std::string, TypeSpecifier> &type){ return op(type.second); }
};

// NOTE: Conversions can only be made between Integer and Floating.
Value::Type MutualCast(Value::Type a, Value::Type b); // Returns Null for uncastable comparison
Value::Type MutualCast(const Value& a, const Value& b);

bool CastValue(const Value &that, Value::Type newtype, Value &to); // Returns true if the conversion is possible.
bool MutualCastValue(Value &a, Value &b, Value::Type t); // This can cause disasters if a and b cannot be converted t.
bool MutualCastValue(Value &a, Value &b);

inline bool TypeIsArithmetic(Value::Type a){ return a==Value::Integer || a==Value::Floating; }
inline bool TypeIsArithmetic(const Value &that){ return TypeIsArithmetic(that.type); }
inline bool TypeIsBitwise(Value::Type a){ return a==Value::Integer; }
inline bool TypeIsBitwise(const Value &that){ return TypeIsBitwise(that.type); }

template<typename T>
inline bool MutualCastIsArithmetic(const T &a, const T &b){ return TypeIsArithmetic(MutualCast(a, b)); }
template<typename T>
inline bool MutualCastIsBitwise(const T &a, const T &b){ return TypeIsBitwise(MutualCast(a, b)); }

template<template<typename> class Op>
bool ValueBinaryOpInteger(Value &that, const Value &other){
    if(that.type!=other.type)
        return false;
    else if(that.type==Value::Integer){
        Op<int64_t> op;
        that.value.integer = op(that.value.integer, other.value.integer);
        return true;
    }
    else
        return false;
}

template<template<typename> class Op>
bool ValueBinaryOpIntegerOrFloating(Value &that, const Value &other){
    if(that.type!=other.type)
        return false;
    else if(that.type==Value::Integer){
        Op<int64_t> op;
        that.value.integer = op(that.value.integer, other.value.integer);
        return true;
    }
    else if(that.type==Value::Floating){
        Op<float> op;
        that.value.floating = op(that.value.floating, other.value.floating);
        return true;
    }
    else
        return false;
}

namespace arith{

template<typename T>
struct bitshiftleft{  inline T operator()(T a, T b) const { return a<<b; } };
template<typename T>
struct bitshiftright{ inline T operator()(T a, T b) const { return a>>b; } };
template<typename T>
struct bitrotateleft{ inline T operator()(T a, T b) const { return a<<b | (a>>(sizeof(T)-b)); } };
template<typename T>
struct bitrotateright{ inline T operator()(T a, T b) const { return a>>b | (a<<(sizeof(T)-b)); } };

} // namespace arith

struct Function{
    std::string::const_iterator start;
    std::vector<std::pair<std::string, TypeSpecifier> > args;
};

std::string ValueName(Value::Type t);

} // namespace Lithium
