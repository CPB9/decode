#pragma once

#include "decode/Config.h"

#include <memory>
#include <string>

namespace decode {

class ParseTree;

std::unique_ptr<ParseTree> parse(const char* fileName);
std::unique_ptr<ParseTree> parse(const std::string& fileName);
}
