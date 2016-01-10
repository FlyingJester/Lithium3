#include "variables.hpp"
#include "context.hpp"
#include <algorithm>

namespace Lithium{

bool VerifyPrototypes(Context &ctx, const TypeSpecifier &type){
    if(type.our_type==Value::Object){
        Value val = ctx.findObject(type.prototype);
        return val.type==Value::Object;
    }
    else if(type.our_type==Value::Function){
        PrototypeVerifier op = {ctx};
        return std::all_of(type.arg_types.cbegin(), type.arg_types.cend(), op);
    }
    else
        return true;
}

bool PrototypeVerifier::op(const TypeSpecifier &type){
    return VerifyPrototypes(ctx, type);
}

std::string ValueName(Value::Type t){
    switch(t){
#define CASE_Z(X_Z) case Value::X_Z: return #X_Z
        CASE_Z(Null);
        CASE_Z(Boolean);
        CASE_Z(Integer);
        CASE_Z(Floating);
        CASE_Z(String);
        CASE_Z(Object);
        CASE_Z(Array);
        CASE_Z(Function);
        default: return "UnknownType";
    }
#undef CASE_Z
}

// Returns Null for uncastable comparison
Value::Type MutualCast(Value::Type a, Value::Type b){
    if(a==b)
        return a;
    if(a==Value::Floating && b==Value::Integer)
        return Value::Floating;
    else if(a==Value::Integer && b==Value::Floating)
        return Value::Floating;
    else
        return Value::Null;
}

Value::Type MutualCast(const Value &a, const Value &b){
    return MutualCast(a.type, b.type);
}

 // Returns true if the conversion is possible
bool CastValue(const Value &that, Value::Type newtype, Value &to){
    if(that.type==newtype){
        to = that;
        return true;
    }
    else if(that.type==Value::Floating && newtype==Value::Integer){
        to.type = Value::Integer;
        to.value.integer = that.value.floating;
        return true;
    }
    else if(that.type==Value::Integer && newtype==Value::Floating){
        to.type = Value::Floating;
        to.value.floating = that.value.integer;
        return true;
    }
    else
        return false;
}

bool MutualCastValue(Value &a, Value &b){
    return MutualCastValue(a, b, MutualCast(a, b));
}

bool MutualCastValue(Value &a, Value &b, Value::Type type){
    if(type==Value::Null)
        return false;
    const bool axt = CastValue(a, type, a) &&
        CastValue(b, type, b);
    assert(axt);
    return axt;
}

} // namespace Lithium
