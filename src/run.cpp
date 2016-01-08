#include "run.hpp"
#include "context.hpp"
#include "interpreter.hpp"

namespace Lithium{

bool runString(const std::string &source){
    Context ctx(source);

    return InterpretProgram(ctx);
}

bool runFile(FILE *file){
    if(!file)
        return false;
    else{
        std::string src;
        char buffer[0x200];
        while(int len = fread(buffer, 1, sizeof(buffer), file)){
            src.append(buffer, len);
        }
        return runString(src);
    }
}

bool runFile(const std::string &path){
    return runFile(fopen(path.c_str(), "r"));
}

} // namespace Lithium

int main(int argc, char *argv[]){
    if(argc>1)
        return Lithium::runFile(argv[1]);
    else{
        char c;
        std::string src;
        while((c=getchar())!=EOF){
            src+=c;
        }
        return Lithium::runString(src);
    }
}
