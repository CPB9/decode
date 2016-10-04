#include "decode/Parser.h"
#include "decode/Grammar.h"

#include <bmcl/FileUtils.h>

namespace decode {

std::unique_ptr<ParseTree> parse(const char* fileName)
{
}

std::unique_ptr<ParseTree> parse(const std::string& fileName)
{
    return parse(fileName.c_str());
}
}
