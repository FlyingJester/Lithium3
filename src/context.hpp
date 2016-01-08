#pragma once
#include <string>
#include <vector>
#include <stack>
#include "variables.hpp"

namespace Lithium{

class Source{
    std::string::const_iterator start, end, at;
    const std::string *src;
    uint64_t line_;

    bool skipComment();

public:
    Source(const std::string &src);
    Source() = delete;
    Source(const Source &that) = default;

    bool valid() const;

    inline std::string::const_iterator position() const { return at; }
    inline void position(std::string::const_iterator &i){ at = i; }

    char getc();
    char peekc() const;

    // Equivalent to `return getc()==c`
    bool match(char c);

    uint64_t line() const;

    bool skipWhitespace();
    bool skipWhitespaceAndNewline();

    static inline bool isWhitespace(char c){
        return c==' ' || c=='\t' || c=='r' || c=='\v';
    }
    static inline bool isAlpha(char c){
        return (c>='a' && c<='z') ||
            (c>='A' && c<='Z') ||
            (c&(1<<7));
    }
    static inline bool isNum(char c){ return (c>='0' && c<='9'); }
    static inline bool isHexNum(char c){
        return isNum(c) ||
            (c>='a' && c<='f') ||
            (c>='A' && c<='F');
    }
    static inline bool isAlphaNum(char c){ return isNum(c) || isAlpha(c); }
    static inline bool isIdent(char c){ return isAlphaNum(c) || c=='_'; }
    static inline bool isQuote(char c){ return c=='"'; }

    static inline bool always_(char c){ if(c){} return true; }
    static inline bool never_(char c){ if(c){} return false; }

    template<bool(*first)(char c), bool(*middle)(char c)>
    bool getString(std::string &str){
        if(!(str.empty() && at!=end && first(*at)))
            return false;

        const std::string::const_iterator l_start = at;
        do{
            at++;
        }while(at!=end && middle(*at));

        str.assign(l_start, at);
        return true;
    }

    inline bool getIdentifier(std::string &str){ return skipWhitespace() && getString<isAlpha, isIdent>(str); }
    inline bool getAlphaIdentifier(std::string &str){ return skipWhitespace() && getString<isAlpha, isAlpha>(str); }
    inline bool getStringLiteral(std::string &str){
        if(!(str.empty() && skipWhitespace() && match('"')))
            return false;
    grab_string:
        {
            std::string x;
            if(!(getString<always_, isQuote>(x) && match('"')))
                return false;
            str.append(std::move(x));
            if(!str.empty() && str.back()=='\\'){
                str.pop_back();
                goto grab_string;
            }
        }
        return true;
    }
};

struct Scope{
    std::string::const_iterator start, end;
    Value scope;
};

class Context{
    std::string src_str_;
    Source src_;
    std::stack<Value, std::vector<Value> > stack;
public:

    // Includes the global scope on the bottom.
    std::vector<Scope> scopes;

    Context(const std::string &source);

    Source &source();
    void push(Value &var);
    Value pop();
    Value &top();

    // Adds to the outer scope
    Value &addVariable(const std::string &name, Value &var);

    struct Error {
        enum Type { NoError, SyntaxError, ReferenceError, TypeError } type;
        uint64_t line;
        std::string what;
    } error;

    typedef Error::Type ErrT;

    inline bool noError() { return error.type!=Error::NoError; }

    inline bool setError(ErrT which, uint64_t line, const std::string &what){
        error.type = which;
        error.line = line;
        error.what = what;
        return false;
    }

    inline bool setError(ErrT which, const std::string &what){ return setError(which, src_.line(), what); }

    static Value findObject(const std::string &name, std::vector<Scope>::iterator i, std::vector<Scope>::iterator end);
    inline Value findObject(const std::string &name){ return findObject(name, scopes.begin(), scopes.end()); }

};

} // namespace Lithium
