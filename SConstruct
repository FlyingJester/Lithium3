import os

environment = Environment()

if os.name=="posix":
    environment.Append(CCFLAGS=" -Wall -Wextra -Werror -pedantic -Os -g ",
        CXXFLAGS=" -std=c++11 -fno-rtti -fno-exceptions ",
        CFLAGS=" -Wno-variadic-macros ",
	LIBS="pthread")

SConscript(dirs=["src"], exports=["environment"])
