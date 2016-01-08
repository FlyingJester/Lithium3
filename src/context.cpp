#include "context.hpp"
#include <cassert>

namespace Lithium{

bool Source::valid() const {
    return at!=end;
}

char Source::getc(){
    if(at==end)
        return 0;

    const char c = *at;
    at++;

    if(c=='\n')
        line_++;

    return c;
}

char Source::peekc() const{
    return (at==end)?'\0':(*at);
}

// Equivalent to `return getc()==c`
bool Source::match(char c){
    return getc()==c;
}

uint64_t Source::line() const{
    return line_;
}

Source::Source(const std::string &s)
  : start(s.cbegin()), end(s.cend()), at(s.cbegin()), src(&s), line_(0llu){

}

bool Source::skipComment(){
    while(at!=end && *at!='\n')
        at++;
    return at!=end;    
}

bool Source::skipWhitespace(){
    while(at!=end){
        switch(*at){
            case ' ': case '\t': case '\r': case '\v':
                at++;
                continue;
            case '%':
                at++;
                skipComment();
                continue;
            default:
                return true;
        }
    }
    return at!=end;
}

bool Source::skipWhitespaceAndNewline(){
    while(at!=end){
        switch(*at){
            case '\n':
                line_++;
            case ' ': case '\t': case '\r': case '\v':
                at++;
                continue;
            case '%':
                at++;
                skipComment();
                continue;
            default:
                return true;
        }
    }
    return at!=end;
}

Context::Context(const std::string &str)
  : src_str_(str), src_(src_str_){

}

Source &Context::source(){
    return src_;
}

void Context::push(Value &var){
    stack.push(var);
}

Value Context::pop(){
    Value val = stack.top();
    stack.pop();
    return std::move(val);
}

Value &Context::top(){
    return stack.top();
}

// Adds to the outer scope
Value &Context::addVariable(const std::string &name, Value &var){
    assert(scopes.back().scope.type==Value::Object);

    scopes.back().scope.value.object->insert({name, var});
    return var;
}

Value Context::findObject(const std::string &name, std::vector<Scope>::iterator i, std::vector<Scope>::iterator end){
    if(i==end)
        return {Value::Null, {}};

    assert(i->scope.type==Value::Object);

    auto x = i->scope.value.object->find(name);
    if(x==i->scope.value.object->cend())
        return findObject(name, i++, end);
    else
        return x->second;
}

} // namespace Lithium
