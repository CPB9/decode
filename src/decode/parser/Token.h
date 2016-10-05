#pragma once

#include "decode/Config.h"
#include "decode/parser/Location.h"
#include "decode/parser/Span.h"

#include <bmcl/StringView.h>

#include <cstddef>

namespace decode {
namespace parser {

enum class TokenKind {
    Invalid = 0,
    DocComment,
    RawComment,
    Whitespace,
    Comma,
    Colon,
    DoubleColon,
    SemiColon,
    LBracket,
    RBracket,
    LBrace,
    RBrace,
    LParen,
    RParen,
    LessThen,
    MoreThen,
    Star,
    Ampersand,
    Hash,
    Equality,
    Slash,
    Exclamation,
    RightArrow,
    Dash,
    Dot,
    Identifier,
    Number,
    Module,
    Import,
    Struct,
    Enum,
    Union,
    Component,
    Parameters,
    Statuses,
    Command,
    Eof,
};

class Token {
public:
    Token(TokenKind kind, const char* start, const char* end, std::size_t line, std::size_t column)
        : _kind(kind)
        , _value(start, end)
        , _loc(line, column)
    {
    }

private:
    TokenKind _kind;
    bmcl::StringView _value;
    Location _loc;
};
}
}
