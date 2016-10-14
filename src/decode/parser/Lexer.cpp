#include "decode/parser/Lexer.h"
#include "decode/parser/Token.h"

#include <bmcl/Logging.h>

#include <pegtl/input.hh>
#include <pegtl/parse.hh>
#include <pegtl/rules.hh>
#include <pegtl/ascii.hh>

namespace decode {
namespace parser {

namespace grammar {

struct Identifier
        : pegtl::identifier {};

struct Eof
        : pegtl::eof {};

struct Eol
        : pegtl::eol {};

// space

struct Blank
        : pegtl::plus<pegtl::one<' ', '\t'>> {};

// keywords

#define KEYWORD_RULE(name, str) \
struct name \
        : pegtl::seq<pegtl_string_t(str), pegtl::at<pegtl::sor<Blank, Eol, Eof>>> {};

KEYWORD_RULE(Module,     "module");
KEYWORD_RULE(Import,     "import");
KEYWORD_RULE(Struct,     "struct");
KEYWORD_RULE(Enum,       "enum");
KEYWORD_RULE(Union,      "union");
KEYWORD_RULE(Component,  "component");
KEYWORD_RULE(Parameters, "parameters");
KEYWORD_RULE(Statuses,   "statuses");
KEYWORD_RULE(Command,    "command");
KEYWORD_RULE(Mut,    "mut");

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
        : pegtl::one<'-'> {};

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

struct RawComment
        : Comment<Slash, Slash> {};

struct DocComment
        : Comment<Slash, Slash, Exclamation> {};

// numbers

struct Number
        : pegtl::plus<pegtl::digit> {};

// grammar

template <typename... A>
struct Helper
        : pegtl::must<pegtl::star<pegtl::sor<A...>>, Eof> {};

struct Grammar
        : Helper<DocComment,
                 RawComment,
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
                 Struct,
                 Enum,
                 Union,
                 Component,
                 Parameters,
                 Statuses,
                 Command,
                 Mut,
                 Identifier,
                 Number,
                 Eof
                 > {};
}

template <typename Rule>
struct Action
        : pegtl::nothing<Rule> {};

#define RULE_TO_TOKEN(name) \
template <> \
struct Action<grammar::name> {\
    static void apply(const pegtl::input& in, std::vector<Token>* tokens)\
    {\
        tokens->emplace_back(TokenKind::name, in.begin(), in.end(), in.line(), in.column());\
    }\
};

RULE_TO_TOKEN(DocComment);
RULE_TO_TOKEN(RawComment);
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
RULE_TO_TOKEN(Dot);
RULE_TO_TOKEN(Number);
RULE_TO_TOKEN(Eol);
RULE_TO_TOKEN(Eof);
RULE_TO_TOKEN(Module);
RULE_TO_TOKEN(Import);
RULE_TO_TOKEN(Struct);
RULE_TO_TOKEN(Enum);
RULE_TO_TOKEN(Union);
RULE_TO_TOKEN(Component);
RULE_TO_TOKEN(Parameters);
RULE_TO_TOKEN(Statuses);
RULE_TO_TOKEN(Command);
RULE_TO_TOKEN(Mut);

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
    pegtl::parse<grammar::Grammar, Action>(data.toStdString(), "", &_tokens);
}

void Lexer::peekNextToken(Token* tok)
{
    if (_nextToken < _tokens.size()) {
        *tok = _tokens[_nextToken];
    } else {
        *tok = _tokens.back();
    }
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
}
