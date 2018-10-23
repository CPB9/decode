/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "decode/parser/Parser.h"

#include "decode/core/FileInfo.h"
#include "decode/core/Diagnostics.h"
#include "decode/core/CfgOption.h"
#include "decode/core/HashMap.h"
#include "decode/core/RangeAttr.h"
#include "decode/core/CmdCallAttr.h"
#include "decode/ast/AllBuiltinTypes.h"
#include "decode/ast/Decl.h"
#include "decode/ast/DocBlock.h"
#include "decode/ast/Ast.h"
#include "decode/ast/Function.h"
#include "decode/parser/Token.h"
#include "decode/ast/Decl.h"
#include "decode/ast/Type.h"
#include "decode/ast/Field.h"
#include "decode/parser/Lexer.h"
#include "decode/ast/ModuleInfo.h"
#include "decode/ast/Type.h"
#include "decode/ast/Component.h"
#include "decode/ast/Constant.h"

#include <bmcl/FileUtils.h>
#include <bmcl/Logging.h>
#include <bmcl/Result.h>
#include <bmcl/Panic.h>

#include <string>
#include <functional>

#define TRY(expr) \
    do { \
        if (!expr) { \
            BMCL_DEBUG() << "failed at:" << __FILE__ << ":" << __LINE__;  \
            return 0; \
        } \
    } while(0);


namespace decode {

#define ADD_BUILTIN_MAP(name, str) \
    _btMap.emplace(str, _builtinTypes->name##Type())

Parser::Parser(Diagnostics* diag)
    : _diag(diag)
    , _builtinTypes(new AllBuiltinTypes)
    , _currentTmMsgNum(0)
{
    ADD_BUILTIN_MAP(usize, "usize");
    ADD_BUILTIN_MAP(isize, "isize");
    ADD_BUILTIN_MAP(varuint, "varuint");
    ADD_BUILTIN_MAP(varint, "varint");
    ADD_BUILTIN_MAP(u8, "u8");
    ADD_BUILTIN_MAP(u16, "u16");
    ADD_BUILTIN_MAP(u32, "u32");
    ADD_BUILTIN_MAP(u64, "u64");
    ADD_BUILTIN_MAP(i8, "i8");
    ADD_BUILTIN_MAP(i16, "i16");
    ADD_BUILTIN_MAP(i32, "i32");
    ADD_BUILTIN_MAP(i64, "i64");
    ADD_BUILTIN_MAP(f32, "f32");
    ADD_BUILTIN_MAP(f64, "f64");
    ADD_BUILTIN_MAP(bool, "bool");
    ADD_BUILTIN_MAP(void, "void");
    ADD_BUILTIN_MAP(char, "char");
}

Parser::~Parser()
{
}

static std::string tokenKindToString(TokenKind kind)
{
    switch (kind) {
    case TokenKind::Invalid:
        return "invalid token";
    case TokenKind::DocComment:
        return "'///'";
    case TokenKind::Blank:
        return "blank";
    case TokenKind::Comma:
        return "','";
    case TokenKind::Colon:
        return "':'";
    case TokenKind::DoubleColon:
        return "'::'";
    case TokenKind::SemiColon:
        return "';'";
    case TokenKind::LBracket:
        return "'['";
    case TokenKind::RBracket:
        return "']'";
    case TokenKind::LBrace:
        return "'{'";
    case TokenKind::RBrace:
        return "'}'";
    case TokenKind::LParen:
        return "'('";
    case TokenKind::RParen:
        return "')'";
    case TokenKind::LessThen:
        return "'<'";
    case TokenKind::MoreThen:
        return "'>'";
    case TokenKind::Star:
        return "'*'";
    case TokenKind::Ampersand:
        return "'&'";
    case TokenKind::Hash:
        return "'#'";
    case TokenKind::Equality:
        return "'='";
    case TokenKind::Slash:
        return "'/'";
    case TokenKind::Exclamation:
        return "'!'";
    case TokenKind::RightArrow:
        return "'->'";
    case TokenKind::Dash:
        return "'-'";
    case TokenKind::Dot:
        return "'.'";
    case TokenKind::DoubleDot:
        return "'..'";
    case TokenKind::Identifier:
        return "identifier";
    case TokenKind::Impl:
        return "'impl'";
    case TokenKind::Number:
        return "number";
    case TokenKind::Module:
        return "'module'";
    case TokenKind::Import:
        return "'import'";
    case TokenKind::Struct:
        return "'struct'";
    case TokenKind::Enum:
        return "'enum'";
    case TokenKind::Variant:
        return "'variant'";
    case TokenKind::Type:
        return "'type'";
    case TokenKind::Component:
        return "'component'";
    case TokenKind::Variables:
        return "'variables'";
    case TokenKind::Statuses:
        return "'statuses'";
    case TokenKind::Events:
        return "'events'";
    case TokenKind::Commands:
        return "'commands'";
    case TokenKind::Parameters:
        return "'parameters'";
    case TokenKind::Autosave:
        return "'autosave'";
    case TokenKind::Fn:
        return "'fn'";
    case TokenKind::UpperFn:
        return "'Fn'";
    case TokenKind::Self:
        return "'self'";
    case TokenKind::Mut:
        return "'mut'";
    case TokenKind::Const:
        return "'const'";
    case TokenKind::True:
        return "'true'";
    case TokenKind::False:
        return "'false'";
    case TokenKind::Eol:
        return "end of line";
    case TokenKind::Eof:
        return "end of file";
    }
    bmcl::panic("unreachable"); //TODO: add macro
}

void Parser::finishSplittingLines()
{
    const char* start = _lastLineStart;
    const char* current = start;
    while (true) {
        char c = *current;
        if (c == '\n') {
            _fileInfo->addLine(start, current);
            start = current;
        }
        if (c == '\0') {
            break;
        }
        current++;
    }
    _lastLineStart = current;
}

void Parser::reportUnexpectedTokenError(TokenKind expected)
{
    std::string msg = "expected " + tokenKindToString(expected);
    reportCurrentTokenError(msg.c_str());
}

ParseResult Parser::parseFile(const char* fname)
{
    bmcl::Result<std::string, int> rv = bmcl::readFileIntoString(fname);
    if (rv.isErr()) {
        return ParseResult();
    }

    Rc<FileInfo> finfo = new FileInfo(std::string(fname), rv.take());
    return parseFile(finfo.get());
}

ParseResult Parser::parseFile(FileInfo* finfo)
{
    cleanup();
    if (parseOneFile(finfo)) {
        finishSplittingLines();
        return _ast;
    }
    finishSplittingLines();
    return ParseResult();
}

void Parser::consume()
{
    _lexer->consumeNextToken(&_currentToken);
}

void Parser::addLine()
{
    const char* current = _currentToken.begin();
    _fileInfo->addLine(_lastLineStart, current);
}

bool Parser::skipCommentsAndSpace()
{
    while (true) {
        switch (_currentToken.kind()) {
        case TokenKind::Hash:
            TRY(parseAttribute());
            break;
        case TokenKind::Blank:
            consume();
            break;
        case TokenKind::Eol:
            addLine();
            consume();
            _lastLineStart = _currentToken.begin();
            break;
//         case TokenKind::RawComment:
//             consume();
//             break;
        case TokenKind::DocComment:
            _docComments.push_back(_currentToken.value());
            consume();
            break;
        case TokenKind::Eof:
            addLine();
            return true;
        case TokenKind::Invalid:
            reportCurrentTokenError("invalid token");
            return false;
        default:
            return true;
        }
    }
    return true;
}

void Parser::skipBlanks()
{
    while (_currentToken.kind() == TokenKind::Blank) {
        consume();
    }
}

void Parser::consumeAndSkipBlanks()
{
    consume();
    skipBlanks();
}

bool Parser::consumeAndExpectCurrentToken(TokenKind expected, const char* msg)
{
    _lexer->consumeNextToken(&_currentToken);
    return expectCurrentToken(expected, msg);
}

bool Parser::consumeAndExpectCurrentToken(TokenKind expected)
{
    _lexer->consumeNextToken(&_currentToken);
    return expectCurrentToken(expected);
}

bool Parser::expectCurrentToken(TokenKind expected)
{
    std::string msg = "expected " + tokenKindToString(expected);
    return expectCurrentToken(expected, msg.c_str());
}

bool Parser::expectCurrentToken(TokenKind expected, const char* msg)
{
    if (_currentToken.kind() != expected) {
        reportCurrentTokenError(msg);
        return false;
    }
    return true;
}

template <typename T>
Rc<T> Parser::beginDecl()
{
    Rc<T> decl = new T;
    decl->_start = _currentToken.location();
    return decl;
}

template <typename T>
void Parser::consumeAndEndDecl(const Rc<T>& decl)
{
    _lexer->consumeNextToken(&_currentToken);
    endDecl(decl);
}

template <typename T>
void Parser::endDecl(const Rc<T>& decl)
{
    decl->_end = _currentToken.location();
    decl->_moduleInfo = _moduleInfo;
}

template <typename T>
Rc<T> Parser::beginType()
{
    Rc<T> type = new T;
    return type;
}

template <typename T>
Rc<T> Parser::beginNamedType(bmcl::StringView name)
{
    Rc<T> type = new T;
    type->setModuleInfo(_moduleInfo.get());
    type->setName(name);
    return type;
}

bool Parser::currentTokenIs(TokenKind kind)
{
    return _currentToken.kind() == kind;
}

Rc<Report> Parser::reportTokenError(Token* tok, const char* msg)
{
    Rc<Report> report = _diag->addReport();
    report->setLocation(_fileInfo.get(), tok->location());
    report->setLevel(Report::Error);
    report->setMessage(msg);
    report->setHighlightMessage(true);
    return report;
}

Rc<Report> Parser::reportCurrentTokenError(const char* msg)
{
    return reportTokenError(&_currentToken, msg);
}

Rc<Report> Parser::reportCurrentTokenError(const std::string& str)
{
    return reportTokenError(&_currentToken, str.c_str());
}

bool Parser::parseModuleDecl()
{
    Rc<DocBlock> docs = createDocsFromComments();
    Location start = _currentToken.location();
    TRY(expectCurrentToken(TokenKind::Module, "every module must begin with module declaration"));
    consume();

    TRY(expectCurrentToken(TokenKind::Blank, "missing blanks after module keyword"));
    consume();

    TRY(expectCurrentToken(TokenKind::Identifier, "module name must be an identifier"));

    bmcl::StringView modName = _currentToken.value();
    _moduleInfo = new ModuleInfo(modName, _fileInfo.get());
    _moduleInfo->setDocs(docs.get());

    Rc<ModuleDecl> modDecl = new ModuleDecl(_moduleInfo.get(), start, _currentToken.location());
    _ast->setModuleDecl(modDecl.get());
    consume();

    clearUnusedDocCommentsAndAttributes();
    return true;
}

void Parser::clearUnusedDocCommentsAndAttributes()
{
    _lastRangeAttr.reset();
    _lastCmdCallAttr.reset();
    _docComments.clear();
}

void Parser::clearGenericParameters()
{
    _currentGenericParameters.clear();
}

bool Parser::parseImports()
{
    while (true) {
        TRY(skipCommentsAndSpace());
        if (_currentToken.kind() != TokenKind::Import) {
            goto end;
        }
        TRY(consumeAndExpectCurrentToken(TokenKind::Blank, "missing blanks after import declaration"));
        TRY(consumeAndExpectCurrentToken(TokenKind::Identifier, "imported module name must be an identifier"));
        Rc<ImportDecl> import = new ImportDecl(_moduleInfo.get(), _currentToken.value());
        TRY(consumeAndExpectCurrentToken(TokenKind::DoubleColon));
        consume();

        auto createImportedTypeFromCurrentToken = [this, import]() {
            Rc<ImportedType> type = new ImportedType(_currentToken.value(), import->path(), _moduleInfo.get());
            if (!import->addType(type.get())) {
                reportCurrentTokenError("duplicate import");
                //TODO: add note - previous import
            }
            consume();
        };

        if (_currentToken.kind() == TokenKind::Identifier) {
            createImportedTypeFromCurrentToken();
        } else if (_currentToken.kind() == TokenKind::LBrace) {
            consume();
            while (true) {
                TRY(expectCurrentToken(TokenKind::Identifier));
                createImportedTypeFromCurrentToken();
                skipBlanks();
                if (_currentToken.kind() == TokenKind::Comma) {
                    consumeAndSkipBlanks();
                    continue;
                } else if (_currentToken.kind() == TokenKind::RBrace) {
                    consume();
                    break;
                } else {
                    reportCurrentTokenError("expected ',' or '}'");
                    return false;
                }
            }
        } else {
            reportCurrentTokenError("expected identifier or '{'");
            return false;
        }

        _ast->addTypeImport(import.get());
    }

end:
    clearUnusedDocCommentsAndAttributes();
    return true;
}

bool Parser::parseTopLevelDecls()
{
    while (true) {
        TRY(skipCommentsAndSpace());

        switch (_currentToken.kind()) {
            case TokenKind::Hash:
                TRY(parseAttribute());
                break;
            case TokenKind::Struct:
                TRY(parseStruct());
                break;
            case TokenKind::Enum:
                TRY(parseEnum());
                break;
            case TokenKind::Variant:
                TRY(parseVariant());
                break;
            case TokenKind::Component:
                TRY(parseComponent());
                break;
            case TokenKind::Impl:
                TRY(parseImplBlock());
                break;
            case TokenKind::Type:
                TRY(parseAlias());
                break;
            case TokenKind::Const:
                TRY(parseConstant());
                break;
            //case TokenKind::Eol:
            //    return true;
            case TokenKind::Eof:
                return true;
            default:
                reportCurrentTokenError("unexpected top level declaration");
                return false;
        }
    }
    return true;
}

bool Parser::parseConstant()
{
    TRY(expectCurrentToken(TokenKind::Const));
    TRY(consumeAndExpectCurrentToken(TokenKind::Blank, "missing blanks after const declaration"));
    skipBlanks();

    TRY(expectCurrentToken(TokenKind::Identifier));
    bmcl::StringView name = _currentToken.value();
    consumeAndSkipBlanks();

    TRY(expectCurrentToken(TokenKind::Colon));
    consumeAndSkipBlanks();

    Token typeToken = _currentToken;
    Rc<Type> type = parseBuiltinOrResolveType();
    if (type.isNull()) {
        return false;
    }
    if (type->typeKind() != TypeKind::Builtin) {
        reportTokenError(&typeToken, "constant can only be of builtin type");
        return false;
    }

    skipBlanks();
    TRY(expectCurrentToken(TokenKind::Equality));
    consumeAndSkipBlanks();

    std::uintmax_t value;
    TRY(parseUnsignedInteger(&value));
    skipBlanks();

    TRY(expectCurrentToken(TokenKind::SemiColon));
    consume();

    _ast->addConstant(new Constant(name, value, type.get()));

    clearUnusedDocCommentsAndAttributes();
    return true;
}

bool Parser::parseAttribute()
{
    TRY(expectCurrentToken(TokenKind::Hash));
    consume();

    TRY(expectCurrentToken(TokenKind::LBracket));
    consumeAndSkipBlanks();

    TRY(expectCurrentToken(TokenKind::Identifier, "expected attribute identifier"));

    if (_currentToken.value() == "cfg") {
        consumeAndSkipBlanks();

        TRY(expectCurrentToken(TokenKind::LParen));
        consumeAndSkipBlanks();

        Rc<CfgOption> opt = parseCfgOption();
        if (opt.isNull()) {
            return false;
        }

        TRY(expectCurrentToken(TokenKind::RParen));
        consumeAndSkipBlanks();
    } else if (_currentToken.value() == "ranges") {
        consumeAndSkipBlanks();

        _lastRangeAttr = parseRangeAttr();
        if (_lastRangeAttr.isNull()) {
            return false;
        }
    } else if (_currentToken.value() == "cmdcall") {
        consumeAndSkipBlanks();

        _lastCmdCallAttr = parseCmdCallAttr();
        if (_lastCmdCallAttr.isNull()) {
            return false;
        }
    } else {
        reportCurrentTokenError("unsupported attribute");
        return false;
    }

    TRY(expectCurrentToken(TokenKind::RBracket));
    consume();
    return true;
}

Rc<RangeAttr> Parser::parseRangeAttr()
{
    Rc<RangeAttr> attr = new RangeAttr;
    bool isOk = parseList(TokenKind::LParen, TokenKind::Comma, TokenKind::RParen, attr, [this](const Rc<RangeAttr>& attr) -> bool {
        TRY(expectCurrentToken(TokenKind::Identifier));
        bmcl::StringView name = _currentToken.value();
        consumeAndSkipBlanks();

        TRY(expectCurrentToken(TokenKind::Equality));
        consumeAndSkipBlanks();

        Token start = _currentToken;
        bool isNegative = false;
        if (currentTokenIs(TokenKind::Dash)) {
            isNegative = true;
            consume();
        }
        TRY(expectCurrentToken(TokenKind::Number));
        consume();

        auto setValue = [&](NumberVariant&& value) -> bool {
            if (name == "default") {
                attr->setDefaultValue(std::move(value));
            } else if (name == "min") {
                attr->setMinValue(std::move(value));
            } else if (name == "max") {
                attr->setMaxValue(std::move(value));
            } else {
                return false;
            }
            return true;
        };

        if (currentTokenIs(TokenKind::Dot)) {
            consume();
            if (currentTokenIs(TokenKind::Number)) {
                consume();
            }

            double value = std::strtod(start.begin(), 0);
            if (errno == ERANGE) {
                errno = 0;
                reportTokenError(&start, "double value range error");
                return false;
            }
            TRY(setValue(NumberVariant(value)));
        } else {
            if (isNegative) {
                std::intmax_t value = std::strtoll(start.begin(), 0, 10);
                if (errno == ERANGE) {
                    errno = 0;
                    reportTokenError(&start, "integer too big");
                    return false;
                }
                TRY(setValue(NumberVariant(value)));
            } else {
                std::uintmax_t value = std::strtoull(start.begin(), 0, 10);
                if (errno == ERANGE) {
                    errno = 0;
                    reportTokenError(&start, "unsigned integer too big");
                    return false;
                }
                TRY(setValue(NumberVariant(value)));
            }
        }

        return true;
    });
    if (!isOk) {
        return nullptr;
    }
    return attr;
}

Rc<CmdCallAttr> Parser::parseCmdCallAttr()
{
    Rc<CmdCallAttr> attr = new CmdCallAttr;

    bool isOk = parseList(TokenKind::LParen, TokenKind::Comma, TokenKind::RParen, attr, [this](const Rc<CmdCallAttr>& attr) -> bool {
        TRY(expectCurrentToken(TokenKind::Identifier));
        bmcl::StringView name = _currentToken.value();
        consumeAndSkipBlanks();

        TRY(expectCurrentToken(TokenKind::Equality));
        consumeAndSkipBlanks();

        TRY(expectCurrentToken(TokenKind::Identifier));
        CmdArgPassKind kind;
        if (_currentToken.value() == "alloc") {
            kind = CmdArgPassKind::AllocPtr;
        } else if (_currentToken.value() == "value") {
            kind = CmdArgPassKind::StackValue;
        } else if (_currentToken.value() == "ptr") {
            kind = CmdArgPassKind::StackPtr;
        } else {
            reportCurrentTokenError("Unknown argument pass kind");
            return false;
        }
        consumeAndSkipBlanks();

        attr->addParam(name, kind);

        return true;
    });
    if (!isOk) {
        return nullptr;
    }
    return attr;
}

Rc<CfgOption> Parser::parseCfgOption()
{
    skipBlanks();
    TRY(expectCurrentToken(TokenKind::Identifier));

    auto cfgListParser = [this](const Rc<AnyCfgOption>& opt) -> bool {
        consumeAndSkipBlanks();

        TRY(parseList(TokenKind::LParen, TokenKind::Comma, TokenKind::RParen, opt, [this](const Rc<AnyCfgOption>& opt) -> bool {
            Rc<CfgOption> nopt = parseCfgOption();
            if (nopt.isNull()) {
                return false;
            }
            opt->addOption(nopt.get());
            return true;
        }));
        return true;
    };

    Rc<CfgOption> opt;
    if (_currentToken.value() == "not") {
        consumeAndSkipBlanks();
        TRY(expectCurrentToken(TokenKind::LParen));
        consumeAndSkipBlanks();

        TRY(expectCurrentToken(TokenKind::Identifier));
        opt = new NotCfgOption(_currentToken.value());
        consumeAndSkipBlanks();

        TRY(expectCurrentToken(TokenKind::RParen));
        consumeAndSkipBlanks();
    } else if (_currentToken.value() == "any") {
        Rc<AnyCfgOption> nopt = new AnyCfgOption;
        TRY(cfgListParser(nopt));
        opt = nopt;
    } else if (_currentToken.value() == "all") {
        Rc<AllCfgOption> nopt = new AllCfgOption;
        TRY(cfgListParser(nopt));
        opt = nopt;
    } else {
        opt = new SingleCfgOption(_currentToken.value());
        consumeAndSkipBlanks();
    }
    skipBlanks();

    return opt;
}

template <typename T>
Rc<T> Parser::parseFunction(bool selfAllowed)
{
    TRY(expectCurrentToken(TokenKind::Fn));
    TRY(consumeAndExpectCurrentToken(TokenKind::Blank, "missing blanks after fn declaration"));
    skipBlanks();

    TRY(expectCurrentToken(TokenKind::Identifier));
    bmcl::StringView name = _currentToken.value();
    Rc<FunctionType> fnType = new FunctionType();
    consume();

    TRY(parseList(TokenKind::LParen, TokenKind::Comma, TokenKind::RParen, fnType, [this, &selfAllowed](const Rc<FunctionType>& func) -> bool {
        if (selfAllowed) {
            if (currentTokenIs(TokenKind::Ampersand)) {
                consumeAndSkipBlanks();

                bool isMut = false;
                if (currentTokenIs(TokenKind::Mut)) {
                    isMut = true;
                    consume();
                    TRY(expectCurrentToken(TokenKind::Blank, "expected blanks before 'self'"));
                    skipBlanks();
                }

                if (currentTokenIs(TokenKind::Self)) {
                    if (isMut) {
                        func->setSelfArgument(SelfArgument::MutReference);
                    } else {
                        func->setSelfArgument(SelfArgument::Reference);
                    }
                    consume();
                    selfAllowed = false;
                    return true;
                } else {
                    reportCurrentTokenError("expected 'self'");
                    return false;
                }
            }

            if (currentTokenIs(TokenKind::Self)) {
                func->setSelfArgument(SelfArgument::Value);
                consume();
                selfAllowed = false;
                return true;
            }
        }

        TRY(expectCurrentToken(TokenKind::Identifier, "expected parameter name"));
        bmcl::StringView argName = _currentToken.value();

        consumeAndSkipBlanks();

        TRY(expectCurrentToken(TokenKind::Colon));

        consumeAndSkipBlanks();

        Rc<Type> type = parseType();
        if (type.isNull()) {
            return false;
        }
        Field* field = new Field(argName, type.get());

        func->addArgument(field);
        selfAllowed = false;
        return true;
    }));

    skipBlanks();

    if (currentTokenIs(TokenKind::RightArrow)) {
        consumeAndSkipBlanks();
        Rc<Type> rType = parseType();
        if (rType.isNull()) {
            return nullptr;
        }

        fnType->setReturnValue(rType.get());
    }
    //TODO: skip past end of line

    _ast->addType(fnType.get());
    return new T(name, fnType.get());
}

bool Parser::parseImplBlock()
{
    TRY(skipCommentsAndSpace());
    TRY(expectCurrentToken(TokenKind::Impl));

    Rc<ImplBlock> block = beginDecl<ImplBlock>();
    consumeAndSkipBlanks();

    Token typeNameToken = _currentToken;
    TRY(expectCurrentToken(TokenKind::Identifier, "expected type name"));

    block->_name = _currentToken.value();
    consumeAndSkipBlanks();

    clearUnusedDocCommentsAndAttributes();
    TRY(parseList(TokenKind::LBrace, TokenKind::Eol, TokenKind::RBrace, block.get(), [this](ImplBlock* block) -> bool {
        Rc<DocBlock> docs = createDocsFromComments();
        Rc<Function> fn = parseFunction<Function>();
        if (fn.isNull()) {
            return false;
        }
        fn->setDocs(docs.get());
        //TODO: check conflicting names
        block->addFunction(fn.get());
        clearUnusedDocCommentsAndAttributes();
        return true;
    }));

    bmcl::OptionPtr<NamedType> type = _ast->findTypeWithName(block->name());
    if (type.isNone()) {
        std::string msg = "no type with name " + typeNameToken.value().toStdString();
        reportTokenError(&typeNameToken, msg.c_str());
        return false;
    }

    //TODO: check conflicts
    _ast->addImplBlock(type.unwrap(), block.get());

    clearUnusedDocCommentsAndAttributes();
    return true;
}

bool Parser::parseAlias()
{
    TRY(skipCommentsAndSpace());
    TRY(expectCurrentToken(TokenKind::Type));
    consume();
    TRY(expectCurrentToken(TokenKind::Blank, "missing blanks after module keyword"));
    skipBlanks();

    TRY(expectCurrentToken(TokenKind::Identifier));
    bmcl::StringView name = _currentToken.value();

    consumeAndSkipBlanks();
    TRY(expectCurrentToken(TokenKind::Equality));

    consumeAndSkipBlanks();

    Rc<Type> link = parseType();
    if (link.isNull()) {
        return false;
    }

    Rc<AliasType> type = new AliasType(name, _moduleInfo.get(), link.get());

    skipBlanks();

    TRY(expectCurrentToken(TokenKind::SemiColon));
    consume();

    //TODO: check conflicts
    _ast->addTopLevelType(type.get());

    clearUnusedDocCommentsAndAttributes();
    return true;
}

Rc<Type> Parser::parseReferenceType()
{
    TRY(skipCommentsAndSpace());
    TRY(expectCurrentToken(TokenKind::Ampersand));
    consume();

    bool isMutable;
    if (_currentToken.kind() == TokenKind::Mut) {
        isMutable = true;
        consume();
    } else {
        isMutable = false;
    }
    TRY(expectCurrentToken(TokenKind::Blank, "missing blanks after module keyword"));
    consumeAndSkipBlanks();

    Rc<Type> pointee;
    if (_currentToken.kind() == TokenKind::LBracket) {
        consumeAndSkipBlanks();
        pointee = parseType();

    } else if (_currentToken.kind() == TokenKind::Identifier) {
        pointee = parseBuiltinOrResolveType();
    } else {
        reportCurrentTokenError("expected identifier or '['");
        return nullptr;
    }

    if (pointee.isNull()) {
        return nullptr;
    }

    Rc<ReferenceType> type = new ReferenceType(ReferenceKind::Reference, isMutable, pointee.get());
    _ast->addType(type.get());
    return type;
}

Rc<Type> Parser::parsePointerType()
{
    TRY(skipCommentsAndSpace());
    TRY(expectCurrentToken(TokenKind::Star));
    consume();

    bool isMutable;
    if (_currentToken.kind() == TokenKind::Mut) {
        isMutable = true;
    } else if (_currentToken.kind() == TokenKind::Const) {
        isMutable = false;
    } else {
        reportCurrentTokenError("expected 'mut' or 'const'");
        return nullptr;
    }
    consumeAndSkipBlanks();

    TRY(skipCommentsAndSpace());
    Rc<Type> pointee;
    if (_currentToken.kind() == TokenKind::Star) {
        pointee = parsePointerType();
    } else {
        pointee = parseBuiltinOrResolveType();
    }

    if (!pointee.isNull()) {
        Rc<ReferenceType> type = new ReferenceType(ReferenceKind::Pointer, isMutable, pointee.get());
        _ast->addType(type.get());
        return type;
    }

    return nullptr;
}

Rc<Type> Parser::parseType()
{
    TRY(skipCommentsAndSpace());

    switch (_currentToken.kind()) {
    case TokenKind::Star:
        return parsePointerType();
    case TokenKind::Ampersand:
        if (_lexer->nextIs(TokenKind::UpperFn)) {
            return parseFunctionPointer();
        }
        if (_lexer->nextIs(TokenKind::LBracket)) {
            return parseDynArrayType();
        }
        return parseReferenceType();
    case TokenKind::LBracket:
        return parseArrayType();
    case TokenKind::Identifier:
        return parseBuiltinOrResolveType();
    default:
        reportCurrentTokenError("error parsing type");
        return nullptr;
    }

    return nullptr;
}

Rc<Type> Parser::parseFunctionPointer()
{
    TRY(expectCurrentToken(TokenKind::Ampersand));

    Rc<FunctionType> fn = new FunctionType();

    consume();
    TRY(expectCurrentToken(TokenKind::UpperFn));
    consume();

    TRY(parseList(TokenKind::LParen, TokenKind::Comma, TokenKind::RParen, fn, [this](const Rc<FunctionType>& fn) {

        Rc<Type> type = parseType();
        if (type.isNull()) {
            return false;
        }
        Field* field = new Field(bmcl::StringView::empty(), type.get());
        fn->addArgument(field);
        return true;
    }));

    skipBlanks();

    if(currentTokenIs(TokenKind::RightArrow)) {
        consumeAndSkipBlanks();
        Rc<Type> rType = parseType();
        if (rType.isNull()) {
            return nullptr;
        }

        fn->setReturnValue(rType.get());
    }
    //TODO: skip past eol

    _ast->addType(fn.get());
    return fn;
}

Rc<Type> Parser::parseDynArrayType()
{
    TRY(expectCurrentToken(TokenKind::Ampersand));
    consume();
    TRY(expectCurrentToken(TokenKind::LBracket));
    consumeAndSkipBlanks();

    Rc<Type> innerType = parseType();
    if (innerType.isNull()) {
        return nullptr;
    }

    TRY(expectCurrentToken(TokenKind::SemiColon));
    consumeAndSkipBlanks();
    TRY(expectCurrentToken(TokenKind::Number, "expected array size"));
    std::uintmax_t maxSize;
    TRY(parseUnsignedInteger(&maxSize));
    skipBlanks();

    TRY(expectCurrentToken(TokenKind::RBracket));
    consume();

    Rc<DynArrayType> type = new DynArrayType(maxSize, innerType.get());
    _ast->addType(type.get());
    return type;
}

Rc<Type> Parser::parseArrayType()
{
    TRY(skipCommentsAndSpace());
    TRY(expectCurrentToken(TokenKind::LBracket));
    consumeAndSkipBlanks();

    Rc<Type> innerType = parseType();
    if (innerType.isNull()) {
        return nullptr;
    }

    skipBlanks();

    TRY(expectCurrentToken(TokenKind::SemiColon));
    consumeAndSkipBlanks();
    TRY(expectCurrentToken(TokenKind::Number, "expected array size"));
    std::uintmax_t elementCount;
    TRY(parseUnsignedInteger(&elementCount));
    skipBlanks();
    TRY(expectCurrentToken(TokenKind::RBracket));
    consume();

    Rc<ArrayType> type = new ArrayType(elementCount, innerType.get());
    _ast->addType(type.get());
    return type;
}

bool Parser::parseUnsignedInteger(std::uintmax_t* dest)
{
    TRY(expectCurrentToken(TokenKind::Number, "error parsing unsigned integer"));

    unsigned long long int value = std::strtoull(_currentToken.begin(), 0, 10);
    if (errno == ERANGE) {
         errno = 0;
         reportCurrentTokenError("unsigned integer too big");
         return false;
    }
    *dest = value;
    consume();
    return true;
}

bool Parser::parseSignedInteger(std::intmax_t* dest)
{
    const char* start = _currentToken.begin();
    if (currentTokenIs(TokenKind::Dash)) {
        consume();
        TRY(expectCurrentToken(TokenKind::Number, "expected integer after sign"));
    }
    TRY(expectCurrentToken(TokenKind::Number, "expected integer"));

    long long int value = std::strtoll(start, 0, 10);
    if (errno == ERANGE) {
        errno = 0;
        reportCurrentTokenError("integer too big");
        return false;
    }
    *dest = value;
    consume();

    return true;
}

Rc<Type> Parser::parseBuiltinOrResolveType()
{
    TRY(expectCurrentToken(TokenKind::Identifier));
    bmcl::StringView name = _currentToken.value();
    consume();

    if (currentTokenIs(TokenKind::LessThen)) {
        RcVec<Type> vec;
        TRY(parseList(TokenKind::LessThen, TokenKind::Comma, TokenKind::MoreThen, &vec, [this](RcVec<Type>* vec){
            Rc<Type> type = parseBuiltinOrResolveType();
            if (type.isNull()) {
                return false;
            }
            vec->push_back(std::move(type));
            return true;
        }));
        auto type = _ast->findTypeWithName(name);
        if (type.isNone()) {
            std::string msg = "No type with name " + name.toStdString();
            reportCurrentTokenError(msg.c_str());
            return nullptr;
        }
        if (type->isImported() || type->isGeneric()) {
            Rc<GenericInstantiationType> generic = new GenericInstantiationType(name, vec, type.unwrap());
            _ast->addGenericInstantiation(generic.get());
            return generic;
        } else {
            std::string msg = "Type " + name.toStdString() + " is not generic";
            reportCurrentTokenError(msg.c_str());
            return nullptr;
        }
    }

    auto it = _btMap.find(name);
    if (it != _btMap.end()) {
        return it->second;
    }
    auto link = _ast->findTypeWithName(name);
    if (link.isSome()) {
        return link.unwrap();
    }
    auto jt = std::find_if(_currentGenericParameters.begin(), _currentGenericParameters.end(), [name](const Rc<GenericParameterType>& type) {
        return type->name() == name;
    });
    if (jt == _currentGenericParameters.end()) {
        std::string msg = "No type with name " + name.toStdString();
        reportCurrentTokenError(msg.c_str());
        return nullptr;
    }
    return *jt;
}

Rc<Field> Parser::parseField()
{
    TRY(expectCurrentToken(TokenKind::Identifier, "expected identifier"));
    Rc<DocBlock> docs = createDocsFromComments();
    bmcl::StringView name = _currentToken.value();
    consumeAndSkipBlanks();
    TRY(expectCurrentToken(TokenKind::Colon));
    consumeAndSkipBlanks();

    Rc<Type> type = parseType();
    if (type.isNull()) {
        return nullptr;
    }
    Rc<Field> field = new Field(name, type.get());
    field->setDocs(docs.get());
    if (!_lastRangeAttr.isNull()) {
        field->setRangeAttribute(_lastRangeAttr.get());
    }
    clearUnusedDocCommentsAndAttributes();
    return field;
}

Rc<DocBlock> Parser::createDocsFromComments()
{
    if (_docComments.empty()) {
        return nullptr;
    }
    Rc<DocBlock> docs = new DocBlock(_docComments);
    _docComments.clear();
    return docs;
}

template <typename T>
bool Parser::parseRecordField(T* parent)
{
    Rc<Field> decl = parseField();
    if (decl.isNull()) {
        return false;
    }
    parent->addField(decl.get());
    return true;
}

bool Parser::parseEnumConstant(EnumType* parent, std::intmax_t* current)
{
    TRY(skipCommentsAndSpace());
    Rc<DocBlock> docs = createDocsFromComments();
    TRY(expectCurrentToken(TokenKind::Identifier));
    bmcl::StringView name = _currentToken.value();
    consumeAndSkipBlanks();

    bool isUserSet;
    if (currentTokenIs(TokenKind::Equality)) {
        consumeAndSkipBlanks();
        TRY(parseSignedInteger(current));
        isUserSet = true;
    } else {
        isUserSet = false;
    }
    Rc<EnumConstant> constant = new EnumConstant(name, *current, isUserSet);
    constant->setDocs(docs.get());
    parent->addConstant(constant.get());

    *current = *current + 1;
    return true;
}

bool Parser::parseVariantField(VariantType* parent)
{
    TRY(skipCommentsAndSpace());
    Rc<DocBlock> docs = createDocsFromComments();
    TRY(expectCurrentToken(TokenKind::Identifier));
    bmcl::StringView name = _currentToken.value();
    consumeAndSkipBlanks();
     //TODO: peek next token

    std::uintmax_t id = parent->fieldsRange().size();
    if (currentTokenIs(TokenKind::Comma)) {
        Rc<ConstantVariantField> field = new ConstantVariantField(id, name);
        field->setDocs(docs.get());
        parent->addField(field.get());
    } else if (currentTokenIs(TokenKind::LBrace)) {
        Rc<StructVariantField> field = new StructVariantField(id, name);
        field->setDocs(docs.get());
        TRY(parseBraceList(field.get(), [this](StructVariantField* dest) {
            return parseRecordField(dest);
        }));
        parent->addField(field.get());
    } else if (currentTokenIs(TokenKind::LParen)) {
        Rc<TupleVariantField> field = new TupleVariantField(id, name);
        field->setDocs(docs.get());
        TRY(parseList(TokenKind::LParen, TokenKind::Comma, TokenKind::RParen, field, [this](const Rc<TupleVariantField>& field) {
            skipBlanks();
            Rc<Type> type = parseType();
            if (type.isNull()) {
                return false;
            }
            field->addType(type.get());
            return true;
        }));
        parent->addField(field.get());
    } else {
        reportCurrentTokenError("expected ',' or '{' or '('");
        return false;
    }

    clearUnusedDocCommentsAndAttributes();
    return true;
}

bool Parser::parseComponentField(Component* parent)
{
    return false;
}
/*
static std::string tokKindToString(TokenKind kind)
{
    switch (kind) {
    }
}*/

template <typename T, typename F, typename... A>
bool Parser::parseList(TokenKind openToken, TokenKind sep, TokenKind closeToken, T&& decl, F&& fieldParser, A&&... args)
{
    TRY(expectCurrentToken(openToken));
    consume();

    return parseList2(sep, closeToken, std::forward<T>(decl), std::forward<F>(fieldParser), std::forward<A>(args)...);
}

template <typename T, typename F, typename... A>
bool Parser::parseList2(TokenKind sep, TokenKind closeToken, T&& decl, F&& fieldParser, A&&... args)
{

    TRY(skipCommentsAndSpace());

    while (true) {
        if (_currentToken.kind() == closeToken) {
            consume();
            return true;
        }

        if (!fieldParser(std::forward<T>(decl), std::forward<A>(args)...)) {
            return false;
        }

        TRY(skipCommentsAndSpace());
        if (_currentToken.kind() == sep) {
            consume();
        }
        TRY(skipCommentsAndSpace());
    }

    return true;
}

template <typename T, typename F, typename... A>
bool Parser::parseBraceList(T&& parent, F&& fieldParser, A&&... args)
{
    return parseList(TokenKind::LBrace, TokenKind::Comma, TokenKind::RBrace, std::forward<T>(parent), std::forward<F>(fieldParser), std::forward<A>(args)...);
}

template <typename T, bool genericAllowed, typename F, typename... A>
bool Parser::parseTag(TokenKind startToken, F&& fieldParser, A&&... args)
{
    TRY(skipCommentsAndSpace());
    Rc<DocBlock> docs = createDocsFromComments();
    TRY(expectCurrentToken(startToken));

    consume();
    TRY(skipCommentsAndSpace());
    TRY(expectCurrentToken(TokenKind::Identifier));

    bmcl::StringView name = _currentToken.value();
    Rc<T> type = new T(name, _moduleInfo.get());
    type->setDocs(docs.get());
    consumeAndSkipBlanks();

    Rc<GenericType> genericType;
    if (currentTokenIs(TokenKind::LessThen)) {
        TRY(parseList(TokenKind::LessThen, TokenKind::Comma, TokenKind::MoreThen, name, [this](bmcl::StringView) -> bool {
            //TODO: check name conflicts
            TRY(expectCurrentToken(TokenKind::Identifier));
            _currentGenericParameters.emplace_back(new GenericParameterType(_currentToken.value(), _moduleInfo.get()));
            consume();
            return true;
        }));
        genericType = new GenericType(name, _currentGenericParameters, type.get());
        skipBlanks();
    }

    TRY(parseBraceList(type.get(), std::forward<F>(fieldParser), std::forward<A>(args)...));
    clearUnusedDocCommentsAndAttributes();
    clearGenericParameters();

    if (!genericType.isNull()) {
        _ast->addTopLevelType(genericType.get());
    } else {
        _ast->addTopLevelType(type.get());
    }
    return true;
}

bool Parser::parseVariant()
{
    return parseTag<VariantType, true>(TokenKind::Variant, std::bind(&Parser::parseVariantField, this, std::placeholders::_1));
}

bool Parser::parseEnum()
{
    std::intmax_t currentConstant = 0;
    return parseTag<EnumType, false>(TokenKind::Enum, std::bind(&Parser::parseEnumConstant, this, std::placeholders::_1, &currentConstant));
}

bool Parser::parseStruct()
{
    return parseTag<StructType, true>(TokenKind::Struct, [this](StructType* decl) {
        if (!parseRecordField(decl)) {
            return false;
        }
        return true;
    });
}

template<typename T, typename F>
bool Parser::parseNamelessTag(TokenKind startToken, TokenKind sep, T* dest, F&& fieldParser)
{
    TRY(expectCurrentToken(startToken));
    consumeAndSkipBlanks();

    TRY(parseList(TokenKind::LBrace, sep, TokenKind::RBrace, dest, std::forward<F>(fieldParser)));
    return true;
}

bool Parser::parseCommands(Component* parent)
{
    TRY(expectCurrentToken(TokenKind::Commands));
    consumeAndSkipBlanks();

    TRY(expectCurrentToken(TokenKind::LBrace));
    consumeAndSkipBlanks();
    TRY(parseList2(TokenKind::Eol, TokenKind::RBrace, parent, [this](Component* comp) {
        Rc<DocBlock> docs = createDocsFromComments();
        Rc<Command> fn = parseFunction<Command>(false);
        if (fn.isNull()) {
            return false;
        }
        if (!_lastCmdCallAttr.isNull()) {
            for (CmdArgument& arg : fn->argumentsRange()) {
                CmdArgPassKind kind = _lastCmdCallAttr->findArgPassKind(arg.field()->name());
                arg.setArgPassKind(kind);
            }
        }
        fn->setDocs(docs.get());
        fn->setNumber(comp->cmdsRange().size());
        comp->addCommand(fn.get());
        clearUnusedDocCommentsAndAttributes();
        return true;
    }));
    clearUnusedDocCommentsAndAttributes();
    return true;
}

bool Parser::parseParameterPath(Component* comp, Parameter* param)
{
    while (true) {
        TRY(expectCurrentToken(TokenKind::Identifier));
        Rc<FieldAccessor> acc = new FieldAccessor(_currentToken.value(), nullptr);
        param->addPathPart(acc.get());
        consume();
        TokenKind kind = _currentToken.kind();
        switch (kind) {
        case TokenKind::Dot:
            consume();
            continue;
        case TokenKind::Comma:
        case TokenKind::RBrace:
        case TokenKind::Blank:
        case TokenKind::Eol:
            return true;
        default:
            reportUnexpectedTokenError(kind);
            return false;
        }
    }

    return false;
}

bool Parser::parseParameter(Component* comp)
{
    TRY(expectCurrentToken(TokenKind::LBrace));
    Token startTok = _currentToken;
    consumeAndSkipBlanks();
    Rc<Parameter> param = new Parameter;
    bool hasName = false;
    TRY(parseList2(TokenKind::Comma, TokenKind::RBrace, param.get(), [this, &hasName, comp](Parameter* param) -> bool {
        skipBlanks();
        TRY(expectCurrentToken(TokenKind::Identifier));
        bmcl::StringView key = _currentToken.value();
        consumeAndSkipBlanks();

        TRY(expectCurrentToken(TokenKind::Colon));
        consumeAndSkipBlanks();

        //TODO: handle key redefinition
        bool flag;
        if (key == "name") {
            TRY(expectCurrentToken(TokenKind::Identifier));
            param->setName(_currentToken.value());
            consume();
            hasName = true;
        } else if (key == "path") {
            TRY(parseParameterPath(comp, param));
        } else if (key == "autosave") {
            TRY(parseBoolean(&flag));
            param->setHasAutoSave(flag);
        } else if (key == "callback") {
            TRY(parseBoolean(&flag));
            param->setHasCallback(flag);
        } else if (key == "readonly") {
            TRY(parseBoolean(&flag));
            param->setReadOnly(flag);
        }

        return true;
    }));

    if (!hasName) {
        reportTokenError(&startTok, "parameter definition lacks 'name'");
        return false;
    }

    if (!comp->addParam(param.get())) {
        std::string msg = "parameter with name '";
        msg.append(param->name().begin(), param->name().end());
        msg += "' already exists";
        reportTokenError(&startTok, msg.c_str());
        return false;
    }

    return true;
}

bool Parser::parseParameters(Component* parent)
{
    TRY(parseNamelessTag(TokenKind::Parameters, TokenKind::Comma, parent, [this](Component* comp) -> bool {
        TRY(parseParameter(comp));

        clearUnusedDocCommentsAndAttributes();
        return true;
    }));
    clearUnusedDocCommentsAndAttributes();
    return true;
}

bool Parser::parseSavedVars(Component* parent)
{
    TRY(parseNamelessTag(TokenKind::Autosave, TokenKind::Comma, parent, [this](Component* comp) -> bool {
        Rc<VarRegexp> re = parseVarRegexp();
        if (re.isNull()) {
            return false;
        }
        comp->addSavedVar(re.get());
        clearUnusedDocCommentsAndAttributes();
        return true;
    }));
    clearUnusedDocCommentsAndAttributes();
    return true;
}

bool Parser::parseComponentImpl(Component* parent)
{
    if(parent->implBlock().isSome()) {
        reportCurrentTokenError("component can have only one impl declaration");
        return false;
    }
    Rc<ImplBlock> impl = new ImplBlock;
    TRY(parseNamelessTag(TokenKind::Impl, TokenKind::Eol, impl.get(), [this](ImplBlock* impl) {
        Rc<DocBlock> docs = createDocsFromComments();
        Rc<Function> fn = parseFunction<Function>(false);
        if (fn.isNull()) {
            return false;
        }
        fn->setDocs(docs.get());
        impl->addFunction(fn.get());
        clearUnusedDocCommentsAndAttributes();
        return true;
    }));
    parent->setImplBlock(impl.get());
    clearUnusedDocCommentsAndAttributes();
    return true;
}

bool Parser::parseVariables(Component* parent)
{
    TRY(parseNamelessTag(TokenKind::Variables, TokenKind::Comma, parent, [this](Component* comp) {
        Rc<Field> field = parseField();
        if (field.isNull()) {
            return false;
        }
        comp->addVar(field.get());
        return true;
    }));

    clearUnusedDocCommentsAndAttributes();
    return true;
}

bool Parser::parseBoolean(bool* value)
{
    if (_currentToken.kind() == TokenKind::True) {
        *value = true;
        consume();
        return true;
    } else if (_currentToken.kind() == TokenKind::False) {
        *value = false;
        consume();
        return true;
    }
    reportCurrentTokenError("expected 'true' or 'false'");
    return false;
}


bool Parser::parseEvents(Component* parent)
{
    TRY(parseNamelessTag(TokenKind::Events, TokenKind::Comma, parent, [this](Component* comp) -> bool {
        TRY(expectCurrentToken(TokenKind::LBracket));
        consumeAndSkipBlanks();

        Token nameToken = _currentToken;
        TRY(expectCurrentToken(TokenKind::Identifier));
        bmcl::StringView name = _currentToken.value();
        consumeAndSkipBlanks();

        TRY(expectCurrentToken(TokenKind::Comma));
        consumeAndSkipBlanks();

        bool isEnabled;
        TRY(parseBoolean(&isEnabled));

        skipBlanks();
        TRY(expectCurrentToken(TokenKind::RBracket));

        EventMsg* msg = new EventMsg(name, _currentTmMsgNum, isEnabled);
        _currentTmMsgNum++;
        bool isOk = comp->addEvent(msg);
        if (!isOk) {
            std::string msg =  "event with name " + name.toStdString() + " already defined";
            reportTokenError(&nameToken, msg.c_str());
            return false;
        }
        consumeAndSkipBlanks();
        TRY(expectCurrentToken(TokenKind::Colon));
        consumeAndSkipBlanks();

        auto parseOne = [this](EventMsg* msg) -> bool {
            Rc<Field> field = parseField();
            if (field.isNull()) {
                return false;
            }
            msg->addField(field.get());
            return true;
        };

        if (currentTokenIs(TokenKind::LBrace)) {
            TRY(parseBraceList(msg, parseOne));
        } else if (currentTokenIs(TokenKind::Identifier)) {
            TRY(parseOne(msg));
        }
        return true;
    }));

    return true;
}

Rc<VarRegexp> Parser::parseVarRegexp()
{
    TRY(expectCurrentToken(TokenKind::Identifier, "regular expression must begin with an identifier"));
    Rc<VarRegexp> re = new VarRegexp;

    while (true) {
        if (currentTokenIs(TokenKind::Identifier)) {
            Rc<FieldAccessor> acc = new FieldAccessor(_currentToken.value(), nullptr);
            re->addAccessor(acc.get());
            consumeAndSkipBlanks();
        } else if (currentTokenIs(TokenKind::LBracket)) {
            Rc<SubscriptAccessor> acc;
            consume();
            uintmax_t m;
            if (currentTokenIs(TokenKind::Number) && _lexer->nextIs(TokenKind::RBracket)) {
                TRY(parseUnsignedInteger(&m));
                acc = new SubscriptAccessor(m, nullptr);
            } else {
                Range range;
                if (currentTokenIs(TokenKind::Number)) {
                    TRY(parseUnsignedInteger(&m));
                    range.lowerBound.emplace(m);
                }

                TRY(expectCurrentToken(TokenKind::DoubleDot));
                consume();

                if (currentTokenIs(TokenKind::Number)) {
                    TRY(parseUnsignedInteger(&m));
                    range.upperBound.emplace(m);
                }
                acc = new SubscriptAccessor(range, nullptr);
            }

            TRY(expectCurrentToken(TokenKind::RBracket));
            consumeAndSkipBlanks();

            re->addAccessor(acc.get());
        }
        skipCommentsAndSpace();
        if (currentTokenIs(TokenKind::Comma) || currentTokenIs(TokenKind::RBrace)) {
            break;
        } else if (currentTokenIs(TokenKind::Dot)) {
            consume();
        }
    }
    return re;
}

bool Parser::parseStatuses(Component* parent)
{
    TRY(parseNamelessTag(TokenKind::Statuses, TokenKind::Comma, parent, [this](Component* comp) -> bool {
        TRY(expectCurrentToken(TokenKind::LBracket));
        consumeAndSkipBlanks();

        Token nameToken = _currentToken;
        TRY(expectCurrentToken(TokenKind::Identifier));
        bmcl::StringView name = _currentToken.value();
        consumeAndSkipBlanks();

        TRY(expectCurrentToken(TokenKind::Comma));
        consumeAndSkipBlanks();

        uintmax_t prio;
        TRY(parseUnsignedInteger(&prio));

        skipBlanks();
        TRY(expectCurrentToken(TokenKind::Comma));
        consumeAndSkipBlanks();

        bool isEnabled;
        TRY(parseBoolean(&isEnabled));

        skipBlanks();
        TRY(expectCurrentToken(TokenKind::RBracket));

        StatusMsg* msg = new StatusMsg(name, _currentTmMsgNum, prio, isEnabled);
        _currentTmMsgNum++;
        bool isOk = comp->addStatus(msg);
        if (!isOk) {
            std::string msg =  "status with name " + name.toStdString() + " already defined";
            reportTokenError(&nameToken, msg.c_str());
            return false;
        }
        consumeAndSkipBlanks();
        TRY(expectCurrentToken(TokenKind::Colon));
        consumeAndSkipBlanks();
        auto parseOneRegexp = [this](StatusMsg* msg) -> bool {
            Rc<VarRegexp> re = parseVarRegexp();
            if (re.isNull()) {
                return false;
            }

            if (re->hasAccessors()) {
                msg->addPart(re.get());
            }
            return true;
        };
        if (currentTokenIs(TokenKind::LBrace)) {
            TRY(parseBraceList(msg, parseOneRegexp));
        } else if (currentTokenIs(TokenKind::Identifier)) {
            TRY(parseOneRegexp(msg));
        }
        return true;
    }));
    clearUnusedDocCommentsAndAttributes();
    return true;
}

//TODO: make component numbers explicit

bool Parser::parseComponent()
{
    TRY(expectCurrentToken(TokenKind::Component));

    if (_ast->component().isSome()) {
        reportCurrentTokenError("only one component declaration is allowed");
        return false;
    }

    Rc<Component> comp = new Component(0, _moduleInfo.get()); //FIXME: make number user-set
    consumeAndSkipBlanks();
    //TRY(expectCurrentToken(TokenKind::Identifier));
    //_ast->addTopLevelType(comp);
    //consumeAndSkipBlanks();

    TRY(expectCurrentToken(TokenKind::LBrace));
    consume();

    while(true) {
        TRY(skipCommentsAndSpace());

        switch(_currentToken.kind()) {
        case TokenKind::Variables: {
            TRY(parseVariables(comp.get()));
            break;
        }
        case TokenKind::Commands: {
            TRY(parseCommands(comp.get()));
            break;
        }
        case TokenKind::Statuses: {
            TRY(parseStatuses(comp.get()));
            break;
        }
        case TokenKind::Events: {
            TRY(parseEvents(comp.get()));
            break;
        }
        case TokenKind::Impl: {
            TRY(parseComponentImpl(comp.get()));
            break;
        }
        case TokenKind::Parameters: {
            TRY(parseParameters(comp.get()));
            break;
        }
        case TokenKind::Autosave: {
            TRY(parseSavedVars(comp.get()));
            break;
        }
        case TokenKind::RBrace:
            consume();
            goto finish;
        default:
            reportCurrentTokenError("invalid component level token");
            return false;
        }
    }

finish:

    _ast->setComponent(comp.get());
    return true;
}

bool Parser::parseOneFile(FileInfo* finfo)
{
    _currentTmMsgNum = 0;
    _fileInfo = finfo;

    _lastLineStart = _fileInfo->contents().c_str();
    _lexer = new Lexer(bmcl::StringView(_fileInfo->contents()));
    _ast = new Ast(_builtinTypes.get());

    _lexer->consumeNextToken(&_currentToken);
    TRY(skipCommentsAndSpace());
    TRY(parseModuleDecl());
    TRY(parseImports());
    TRY(parseTopLevelDecls());

    return true;
}

void Parser::cleanup()
{
    _docComments.clear();
    _lexer = nullptr;
    _fileInfo = nullptr;
    _moduleInfo = nullptr;
    _ast = nullptr;
}

Location Parser::currentLoc() const
{
    return _currentToken.location();
}
}




