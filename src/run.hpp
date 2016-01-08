#pragma once
#include <cstdio>
#include <string>

namespace Lithium{

bool runString(const std::string &string);
bool runFile(FILE *file);
bool runFile(const std::string &path);

} // namespace Lithium

int main(int argc, char *argv[]);
