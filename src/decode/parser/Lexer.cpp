/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "decode/parser/Lexer.h"
#include "decode/parser/Token.h"

#include <bmcl/Logging.h>

#include <tao/pegtl/memory_input.hpp>
#include <tao/pegtl/parse.hpp>
#include <tao/pegtl/rules.hpp>
#include <tao/pegtl/ascii.hpp>
#include <tao/pegtl/utf8.hpp>

using namespace tao; //temp

namespace decode {

namespace grammar {

struct Identifier
        : pegtl::identifier {};

struct Eol
        : pegtl::eol {};

// space

struct Blank
        : pegtl::plus<pegtl::one<' ', '\t'>> {};

// keywords

#define KEYWORD_RULE(name, str) \
struct name \
        : pegtl::seq<TAOCPP_PEGTL_STRING(str), pegtl::not_at<pegtl::alnum>> {};

KEYWORD_RULE(Module,     "module");
KEYWORD_RULE(Import,     "import");
KEYWORD_RULE(Struct,     "struct");
KEYWORD_RULE(Enum,       "enum");
KEYWORD_RULE(Variant,    "variant");
KEYWORD_RULE(Type,       "type");
KEYWORD_RULE(Component,  "component");
KEYWORD_RULE(Variables,  "variables");
KEYWORD_RULE(Statuses,   "statuses");
KEYWORD_RULE(Events,     "events");
KEYWORD_RULE(Commands,   "commands");
KEYWORD_RULE(Parameters, "parameters");
KEYWORD_RULE(Autosave,   "autosave");
KEYWORD_RULE(Mut,        "mut");
KEYWORD_RULE(Const,      "const");
KEYWORD_RULE(Impl,       "impl");
KEYWORD_RULE(Fn,         "fn");
KEYWORD_RULE(UpperFn,    "Fn");
KEYWORD_RULE(Self,       "self");
KEYWORD_RULE(True,       "true");
KEYWORD_RULE(False,      "false");

// chars

struct Comma
        : pegtl::one<','> {};

struct Colon
        : pegtl::one<':'> {};

struct DoubleColon
        : pegtl::two<':'> {};

struct SemiColon
        : pegtl::one<';'> {};

struct LBracket
        : pegtl::one<'['> {};

struct RBracket
        : pegtl::one<']'> {};

struct LBrace
        : pegtl::one<'{'> {};

struct RBrace
        : pegtl::one<'}'> {};

struct LParen
        : pegtl::one<'('> {};

struct RParen
        : pegtl::one<')'> {};

struct LessThen
        : pegtl::one<'<'> {};

struct MoreThen
        : pegtl::one<'>'> {};

struct Star
        : pegtl::one<'*'> {};

struct Ampersand
        : pegtl::one<'&'> {};

struct Hash
        : pegtl::one<'#'> {};

struct Equality
        : pegtl::one<'='> {};

struct Slash
        : pegtl::one<'/'> {};

struct Exclamation
        : pegtl::one<'!'> {};

struct Dash
        //: pegtl::one<'-'> {};
        : pegtl::seq<pegtl::one<'-'>, pegtl::not_at<pegtl::one<'>'>>> {};

struct RightArrow
        : pegtl::string<'-', '>'> {};

struct Dot
        : pegtl::one<'.'> {};

struct DoubleDot
        : pegtl::two<'.'> {};

// comments

template <typename... A>
struct Comment
        : pegtl::disable<pegtl::seq<A..., pegtl::until<pegtl::eolf>>> {};

struct DocComment
        : Comment<Slash, Slash, Slash> {};

// numbers

struct Number
        : pegtl::plus<pegtl::digit> {};

// eof

struct Eof
        : pegtl::eof {};

// grammar

template <typename... A>
struct Helper
        : pegtl::must<pegtl::plus<pegtl::sor<A...>>, Eof> {};

struct Grammar
        : Helper<DocComment,
                 Blank,
                 Eol,
                 Comma,
                 DoubleColon,
                 Colon,
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
                 DoubleDot,
                 Dot,
                 Module,
                 Import,
                 Impl,
                 Struct,
                 Self,
                 Enum,
                 Variant,
                 Type,
                 Component,
                 Variables,
                 Statuses,
                 Events,
                 Commands,
                 Parameters,
                 Autosave,
                 Mut,
                 Fn,
                 UpperFn,
                 Const,
                 True,
                 False,
                 Identifier,
                 Number
                 > {};
}

template <typename Rule>
struct Action
        : pegtl::nothing<Rule> {};

#define RULE_TO_TOKEN(name) \
template <> \
struct Action<grammar::name> {\
    template<typename Input>\
    static void apply(const Input& in, std::vector<Token>* tokens)\
    {\
        tokens->emplace_back(TokenKind::name, in.begin(), in.end(), in.position().line, in.position().byte_in_line + 1);\
    }\
};

RULE_TO_TOKEN(DocComment);
//RULE_TO_TOKEN(RawComment);
RULE_TO_TOKEN(Comma);
RULE_TO_TOKEN(DoubleColon);
RULE_TO_TOKEN(Colon);
RULE_TO_TOKEN(SemiColon);
RULE_TO_TOKEN(Blank);
RULE_TO_TOKEN(LBracket);
RULE_TO_TOKEN(RBracket);
RULE_TO_TOKEN(LBrace);
RULE_TO_TOKEN(RBrace);
RULE_TO_TOKEN(LParen);
RULE_TO_TOKEN(RParen);
RULE_TO_TOKEN(Identifier);
RULE_TO_TOKEN(LessThen);
RULE_TO_TOKEN(MoreThen);
RULE_TO_TOKEN(Star);
RULE_TO_TOKEN(Ampersand);
RULE_TO_TOKEN(Hash);
RULE_TO_TOKEN(Equality);
RULE_TO_TOKEN(Slash);
RULE_TO_TOKEN(Exclamation);
RULE_TO_TOKEN(RightArrow);
RULE_TO_TOKEN(Dash);
RULE_TO_TOKEN(DoubleDot);
RULE_TO_TOKEN(Dot);
RULE_TO_TOKEN(Number);
RULE_TO_TOKEN(Eol);
RULE_TO_TOKEN(Module);
RULE_TO_TOKEN(Import);
RULE_TO_TOKEN(Impl);
RULE_TO_TOKEN(Fn);
RULE_TO_TOKEN(UpperFn);
RULE_TO_TOKEN(Self);
RULE_TO_TOKEN(Struct);
RULE_TO_TOKEN(Enum);
RULE_TO_TOKEN(Variant);
RULE_TO_TOKEN(Type);
RULE_TO_TOKEN(Component);
RULE_TO_TOKEN(Variables);
RULE_TO_TOKEN(Statuses);
RULE_TO_TOKEN(Events);
RULE_TO_TOKEN(Commands);
RULE_TO_TOKEN(Parameters);
RULE_TO_TOKEN(Autosave);
RULE_TO_TOKEN(Mut);
RULE_TO_TOKEN(Const);
RULE_TO_TOKEN(True);
RULE_TO_TOKEN(False);
RULE_TO_TOKEN(Eof);

Lexer::Lexer()
    : _nextToken(0)
{
    _tokens.emplace_back();
}

Lexer::Lexer(bmcl::StringView data)
{
    reset(data);
}

void Lexer::reset(bmcl::StringView data)
{
    _tokens.clear();
    _data = data;
    _nextToken = 0;
    //TODO: catch pegtl exceptions
    pegtl::memory_input<> input(data.begin(), data.size(), "");
    try {
        pegtl::parse<grammar::Grammar, Action>(input, &_tokens);
    } catch (const pegtl::parse_error& err) {
        for (const pegtl::position& info : err.positions) {
            _tokens.emplace_back(TokenKind::Invalid, info.source.data(), info.source.data() + info.source.size(), info.line, info.byte_in_line + 1);
        }
    }
}

void Lexer::peekNextToken(Token* tok)
{
    if (_nextToken < _tokens.size()) {
        *tok = _tokens[_nextToken];
    } else {
        *tok = _tokens.back(); //eol
    }
}

bool Lexer::nextIs(TokenKind kind)
{
    if (_nextToken < _tokens.size()) {
        return kind == _tokens[_nextToken].kind();
    }
    return kind == TokenKind::Eol;
}

void Lexer::consumeNextToken(Token* tok)
{
    if (_nextToken < _tokens.size()) {
        *tok = _tokens[_nextToken];
        _nextToken++;
    } else {
        *tok = _tokens.back();
    }
}
}
