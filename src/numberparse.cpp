#include "numberparse.hpp"
#include "context.hpp"

namespace Lithium{

static inline bool is_oct(char c){
    return (c>='0' && c<='7');
}

static inline bool is_bin(char c){
    return (c=='0' || c=='1');
}

static uint_fast8_t hex_from_digit(char c){
    if(Source::isNum(c))
        return c-'0';
    else if(c>='a' && c<='f')
        return 10+(c-'a');
    else if(c>='A' && c<='F')
        return 10+(c-'A');
    else
        return 0;
}

struct l_complex { int64_t n; uint64_t d; };

// Returns {x, y} where val = "$x.$y"
static l_complex number_literal(Source &src){
    l_complex that = { 0ll, 0llu };
    bool negative = false;
    if(src.peekc()=='-'){
        negative = true;
        src.getc();
    }

    const char c1 = src.getc();
    if(c1=='0'){
        const char c2 = src.getc();
        if(c2=='x' || c2=='X'){
            char c3;
            while(Source::isHexNum(c3 = src.peekc())){
                that.n<<=4;
                that.n += hex_from_digit(c3);
                src.getc();
            }
        }
        else{
            char c3;
            that.n = c2 - '0';
            while(Source::isHexNum(c3 = src.peekc())){
                that.n<<=3;
                that.n += c3 - '0';
                src.getc();
            }
        }
    }
    // Skip a check for is_numeric(c1). Garbage in, garbage out.
    else{
        that.n = c1;
        char c3;
        while(Source::isNum(c3 = src.peekc())){
            that.n *= 10;
            that.n += c3 - '0';
            src.getc();
        }

        if(c3=='.'){
            src.getc();
            while(Source::isNum(c3 = src.peekc())){
                that.d *= 10;
                that.d += c3 - '0';
                src.getc();
            }
        }
    }
    if(negative)
        that.n= -that.n;
    return that;
}

static double rasterize_complex(int64_t a, uint64_t b){
    if(b==0){
        return a;
    }
    else{
        // log10 is way slower than this. Probably something to do with standards.
        uint_fast16_t digits = 0;
        uint64_t z = b;
        assert(z);
        while(z){
            digits++;
            z/=10;
        }
        double dec = (double)b / ((double)digits*10.0);
        return dec + (double)a;
    }
}

static double rasterize_complex(const l_complex &that){
    return rasterize_complex(that.n, that.d);
}

bool ParseNumberLiteral(Context &ctx, Value &to){
    if(!Source::isNum(ctx.source().peekc()))
        return false;
    l_complex that = number_literal(ctx.source());
    if(that.d==0llu){
        to.type = Value::Integer;
        to.value.integer = that.n;
    }
    else{
        to.type = Value::Floating;
        to.value.floating = rasterize_complex(that);
    }
    return true;
}

} // namespace Lithium
