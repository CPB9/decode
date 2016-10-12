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
    Blank,
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
    Eol,
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

    Token()
        : _kind(TokenKind::Invalid)
        , _loc(0, 0)
    {
    }

    const char* begin() const
    {
        return _value.begin();
    }

    std::size_t line() const
    {
        return _loc.line;
    }

    std::size_t column() const
    {
        return _loc.column;
    }

    const Location& location() const
    {
        return _loc;
    }

    TokenKind kind() const
    {
        return _kind;
    }

    bmcl::StringView value() const
    {
        return _value;
    }

private:
    TokenKind _kind;
    bmcl::StringView _value;
    Location _loc;
};
}
}
