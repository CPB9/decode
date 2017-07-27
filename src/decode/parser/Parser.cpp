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
#include "decode/parser/Decl.h"
#include "decode/parser/DocBlock.h"
#include "decode/parser/Ast.h"
#include "decode/parser/Token.h"
#include "decode/parser/Decl.h"
#include "decode/parser/Type.h"
#include "decode/parser/Lexer.h"
#include "decode/parser/ModuleInfo.h"
#include "decode/parser/Type.h"
#include "decode/parser/Component.h"
#include "decode/parser/Constant.h"

#include <bmcl/FileUtils.h>
#include <bmcl/Logging.h>
#include <bmcl/Result.h>

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

#define ADD_BUILTIN(name, type, str) \
    _builtinTypes->name = new BuiltinType(BuiltinTypeKind::type); \
    _builtinTypes->btMap.emplace(str, _builtinTypes->name)

Parser::Parser(Diagnostics* diag)
    : _diag(diag)
    , _builtinTypes(new AllBuiltinTypes)
{
    ADD_BUILTIN(usizeType, USize, "usize");
    ADD_BUILTIN(isizeType, ISize, "isize");
    ADD_BUILTIN(varuintType, Varuint, "varuint");
    ADD_BUILTIN(varintType, Varint, "varint");
    ADD_BUILTIN(u8Type, U8, "u8");
    ADD_BUILTIN(i8Type, I8, "i8");
    ADD_BUILTIN(u16Type, U16, "u16");
    ADD_BUILTIN(i16Type, I16, "i16");
    ADD_BUILTIN(u32Type, U32, "u32");
    ADD_BUILTIN(i32Type, I32, "i32");
    ADD_BUILTIN(u64Type, U64, "u64");
    ADD_BUILTIN(i64Type, I64, "i64");
    ADD_BUILTIN(i64Type, F32, "f32");
    ADD_BUILTIN(i64Type, F64, "f64");
    ADD_BUILTIN(boolType, Bool, "bool");
    ADD_BUILTIN(voidType, Void, "void");
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
    case TokenKind::Parameters:
        return "'parameters'";
    case TokenKind::Statuses:
        return "'statuses'";
    case TokenKind::Commands:
        return "'commands'";
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
    case TokenKind::Eol:
        return "end of line";
    case TokenKind::Eof:
        return "end of file";
    }
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
            _docComments.push_back(_currentToken.value().trim());
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
    Rc<TypeDecl> decl = beginDecl<TypeDecl>();
    Rc<T> type = new T;
    decl->_type = type;
    //_ast->_typeToDecl.emplace(type, decl);
    _typeDeclStack.push_back(decl);
    return type;
}

template <typename T>
Rc<T> Parser::beginNamedType(bmcl::StringView name)
{
    Rc<TypeDecl> decl = beginDecl<TypeDecl>();
    Rc<T> type = new T;
    type->setModuleInfo(_moduleInfo.get());
    type->setName(name);
    decl->_type = type;
    //_ast->_typeToDecl.emplace(type, decl);
    _typeDeclStack.push_back(decl);
    return type;
}

template <typename T>
void Parser::consumeAndEndType(const Rc<T>& type)
{
    consume();
    endType(type);
}

template <typename T>
void Parser::endType(const Rc<T>& type)
{
    assert(!_typeDeclStack.empty());
    //assert(_typeDeclStack.back()->type() == type);
    endDecl(_typeDeclStack.back());
    _typeDeclStack.pop_back();
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
    return report;
}

Rc<Report> Parser::reportCurrentTokenError(const char* msg)
{
    return reportTokenError(&_currentToken, msg);
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

    clearUnusedDocComments();
    return true;
}

void Parser::clearUnusedDocComments()
{
    _docComments.clear();
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
        Rc<TypeImport> import = new TypeImport(_currentToken.value());
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
    clearUnusedDocComments();
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
    if (!type) {
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

    clearUnusedDocComments();
    return true;
}

bool Parser::parseAttribute()
{
    TRY(skipCommentsAndSpace());
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
        if (!opt) {
            return false;
        }

        TRY(expectCurrentToken(TokenKind::RParen));
        consumeAndSkipBlanks();
    } else {
        reportCurrentTokenError("only cfg attributes are supported");
        return false;
    }

    TRY(expectCurrentToken(TokenKind::RBracket));
    consume();
    return true;
}

Rc<CfgOption> Parser::parseCfgOption()
{
    skipBlanks();
    TRY(expectCurrentToken(TokenKind::Identifier));

    auto cfgListParser = [this](const Rc<AnyCfgOption>& opt) -> bool {
        consumeAndSkipBlanks();

        TRY(parseList(TokenKind::LParen, TokenKind::Comma, TokenKind::RParen, opt, [this](const Rc<AnyCfgOption>& opt) -> bool {
            Rc<CfgOption> nopt = parseCfgOption();
            if (!nopt) {
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

Rc<Function> Parser::parseFunction(bool selfAllowed)
{
    TRY(expectCurrentToken(TokenKind::Fn));
    TRY(consumeAndExpectCurrentToken(TokenKind::Blank, "missing blanks after fn declaration"));
    skipBlanks();

    TRY(expectCurrentToken(TokenKind::Identifier));
    bmcl::StringView name = _currentToken.value();
    Rc<FunctionType> fnType = new FunctionType(_moduleInfo.get());
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
        if (!type) {
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
        if (!rType) {
            return nullptr;
        }

        fnType->setReturnValue(rType.get());
    }
    //TODO: skip past end of line

    return new Function(name, fnType.get());
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

    clearUnusedDocComments();
    TRY(parseList(TokenKind::LBrace, TokenKind::Eol, TokenKind::RBrace, block.get(), [this](ImplBlock* block) -> bool {
        Rc<DocBlock> docs = createDocsFromComments();
        Rc<Function> fn = parseFunction();
        fn->setDocs(docs.get());
        if (!fn) {
            return false;
        }
        //TODO: check conflicting names
        block->addFunction(fn.get());
        clearUnusedDocComments();
        return true;
    }));

    bmcl::OptionPtr<NamedType> type = _ast->findTypeWithName(block->name());
    if (type.isNone()) {
        std::string msg = "no type with name " + typeNameToken.value().toStdString();
        reportTokenError(&typeNameToken, msg.c_str());
        return false;
    }

    //TODO: check conflicts
    _ast->addImplBlock(block.get());

    clearUnusedDocComments();
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
    if (!link) {
        return false;
    }

    Rc<AliasType> type = new AliasType(name, _moduleInfo.get(), link.get());

    skipBlanks();

    TRY(expectCurrentToken(TokenKind::SemiColon));
    consume();

    //TODO: check conflicts
    _ast->addTopLevelType(type.get());

    clearUnusedDocComments();
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

    if (!pointee) {
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

    if (pointee) {
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
            return parseSliceType();
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

    Rc<FunctionType> fn = new FunctionType(_moduleInfo.get());

    consume();
    TRY(expectCurrentToken(TokenKind::UpperFn));
    consume();

    TRY(parseList(TokenKind::LParen, TokenKind::Comma, TokenKind::RParen, fn, [this](const Rc<FunctionType>& fn) {

        Rc<Type> type = parseType();
        if (!type) {
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
        if (!rType) {
            return nullptr;
        }

        fn->setReturnValue(rType.get());
    }
    //TODO: skip past eol

    _ast->addType(fn.get());
    return fn;
}

Rc<Type> Parser::parseSliceType()
{
    TRY(expectCurrentToken(TokenKind::Ampersand));
    consume();
    TRY(expectCurrentToken(TokenKind::LBracket));
    consumeAndSkipBlanks();

    Rc<Type> innerType = parseType();
    if (!innerType) {
        return nullptr;
    }

    TRY(expectCurrentToken(TokenKind::RBracket));
    consume();

    Rc<SliceType> type = new SliceType(_moduleInfo.get(), innerType.get());
    _ast->addType(type.get());
    return type;
}

Rc<Type> Parser::parseArrayType()
{
    TRY(skipCommentsAndSpace());
    TRY(expectCurrentToken(TokenKind::LBracket));
    consumeAndSkipBlanks();

    Rc<Type> innerType = parseType();
    if (!innerType) {
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
    bool isNegative = false;
    const char* start = _currentToken.begin();
    if (currentTokenIs(TokenKind::Dash)) {
        isNegative = true;
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
    auto it = _builtinTypes->btMap.find(_currentToken.value());
    if (it != _builtinTypes->btMap.end()) {
        consume();
        return it->second;
    }
    auto link = _ast->findTypeWithName(_currentToken.value());
    if (link.isNone()) {
        std::string msg = "no type with name " + _currentToken.value().toStdString();
        reportCurrentTokenError(msg.c_str());
        return nullptr;
    }
    consume();
    return link.unwrap();
}

Rc<Field> Parser::parseField()
{
    TRY(expectCurrentToken(TokenKind::Identifier, "expected identifier"));
    bmcl::StringView name = _currentToken.value();
    consumeAndSkipBlanks();
    TRY(expectCurrentToken(TokenKind::Colon));
    consumeAndSkipBlanks();

    Rc<Type> type = parseType();
    if (!type) {
        return nullptr;
    }
    Rc<Field> field = new Field(name, type.get());

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
    Rc<DocBlock> docs = createDocsFromComments();
    Rc<Field> decl = parseField();
    decl->setDocs(docs.get());
    if (!decl) {
        return false;
    }
    parent->addField(decl.get());
    return true;
}

bool Parser::parseEnumConstant(EnumType* parent)
{
    TRY(skipCommentsAndSpace());
    Rc<DocBlock> docs = createDocsFromComments();
    TRY(expectCurrentToken(TokenKind::Identifier));
    bmcl::StringView name = _currentToken.value();
    consumeAndSkipBlanks();

    if (currentTokenIs(TokenKind::Equality)) {
        consumeAndSkipBlanks();
        intmax_t value;
        TRY(parseSignedInteger(&value));
        //TODO: check value
        Rc<EnumConstant> constant = new EnumConstant(name, value, true);
        constant->setDocs(docs.get());
        if (!parent->addConstant(constant.get())) {
            reportCurrentTokenError("enum constant redefinition");
            return false;
        }
    } else {
        reportCurrentTokenError("expected '='");
        return false;
    }

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

    if (currentTokenIs(TokenKind::Comma)) {
        Rc<ConstantVariantField> field = new ConstantVariantField(name);
        field->setDocs(docs.get());
        parent->addField(field.get());
    } else if (currentTokenIs(TokenKind::LBrace)) {
        Rc<StructVariantField> field = new StructVariantField(name);
        field->setDocs(docs.get());
        TRY(parseBraceList(field.get(), [this](StructVariantField* dest) {
            return parseRecordField(dest);
        }));
        parent->addField(field.get());
    } else if (currentTokenIs(TokenKind::LParen)) {
        Rc<TupleVariantField> field = new TupleVariantField(name);
        field->setDocs(docs.get());
        TRY(parseList(TokenKind::LParen, TokenKind::Comma, TokenKind::RParen, field, [this](const Rc<TupleVariantField>& field) {
            skipBlanks();
            Rc<Type> type = parseType();
            if (!type) {
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

    clearUnusedDocComments();
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

template <typename T, typename F>
bool Parser::parseList(TokenKind openToken, TokenKind sep, TokenKind closeToken, T&& decl, F&& fieldParser)
{
    TRY(expectCurrentToken(openToken));
    consume();

    TRY(skipCommentsAndSpace());

    while (true) {
        if (_currentToken.kind() == closeToken) {
            consume();
            return true;
        }

        if (!fieldParser(std::forward<T>(decl))) {
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

template <typename T, typename F>
bool Parser::parseBraceList(T&& parent, F&& fieldParser)
{
    return parseList(TokenKind::LBrace, TokenKind::Comma, TokenKind::RBrace, std::forward<T>(parent), std::forward<F>(fieldParser));
}

template <typename T, bool genericAllowed, typename F>
bool Parser::parseTag(TokenKind startToken, F&& fieldParser)
{
    TRY(skipCommentsAndSpace());
    TRY(expectCurrentToken(startToken));

    consume();
    TRY(skipCommentsAndSpace());
    TRY(expectCurrentToken(TokenKind::Identifier));
    Rc<T> type = beginNamedType<T>(_currentToken.value());
    consumeAndSkipBlanks();

    TRY(parseBraceList(type, std::forward<F>(fieldParser)));

    _ast->addTopLevelType(type.get());
    endType(type);
    return true;
}

template <typename T, bool genericAllowed, typename F>
bool Parser::parseTag2(TokenKind startToken, F&& fieldParser)
{
    TRY(skipCommentsAndSpace());
    Rc<DocBlock> docs = createDocsFromComments();
    TRY(expectCurrentToken(startToken));

    consume();
    TRY(skipCommentsAndSpace());
    TRY(expectCurrentToken(TokenKind::Identifier));
    Rc<T> type = new T(_currentToken.value(), _moduleInfo.get());
    type->setDocs(docs.get());
    consumeAndSkipBlanks();

    TRY(parseBraceList(type.get(), std::forward<F>(fieldParser)));
    clearUnusedDocComments();

    _ast->addTopLevelType(type.get());
    return true;
}

bool Parser::parseVariant()
{
    return parseTag2<VariantType, true>(TokenKind::Variant, std::bind(&Parser::parseVariantField, this, std::placeholders::_1));
}

bool Parser::parseEnum()
{
    return parseTag2<EnumType, false>(TokenKind::Enum, std::bind(&Parser::parseEnumConstant, this, std::placeholders::_1));
}

bool Parser::parseStruct()
{
    return parseTag2<StructType, true>(TokenKind::Struct, [this](StructType* decl) {
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
    if(parent->hasCmds()) {
        reportCurrentTokenError("component can have only one commands declaration");
        return false;
    }
    TRY(parseNamelessTag(TokenKind::Commands, TokenKind::Eol, parent, [this](Component* comp) {
        Rc<DocBlock> docs = createDocsFromComments();
        Rc<Function> fn = parseFunction(false);
        fn->setDocs(docs.get());
        if (!fn) {
            return false;
        }
        comp->addCommand(fn.get());
        clearUnusedDocComments();
        return true;
    }));
    clearUnusedDocComments();
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
        Rc<Function> fn = parseFunction(false);
        fn->setDocs(docs.get());
        if (!fn) {
            return false;
        }
        impl->addFunction(fn.get());
        clearUnusedDocComments();
        return true;
    }));
    parent->setImplBlock(impl.get());
    clearUnusedDocComments();
    return true;
}

bool Parser::parseParameters(Component* parent)
{
    if(parent->hasParams()) {
        reportCurrentTokenError("component can have only one parameters declaration");
        return false;
    }
    TRY(parseNamelessTag(TokenKind::Parameters, TokenKind::Comma, parent, [this](Component* comp) {
        Rc<DocBlock> docs = createDocsFromComments();
        Rc<Field> field = parseField();
        field->setDocs(docs.get());
        clearUnusedDocComments();
        if (!field) {
            return false;
        }
        comp->addParam(field.get());
        return true;
    }));

    clearUnusedDocComments();
    return true;
}

bool Parser::parseStatuses(Component* parent)
{
    if(parent->hasStatuses()) {
        reportCurrentTokenError("component can have only one statuses declaration");
        return false;
    }
    TRY(parseNamelessTag(TokenKind::Statuses, TokenKind::Comma, parent, [this](Component* comp) -> bool {
        TRY(expectCurrentToken(TokenKind::LBracket));
        consumeAndSkipBlanks();

        Token numToken = _currentToken;
        uintmax_t num;
        TRY(parseUnsignedInteger(&num));

        skipBlanks();
        TRY(expectCurrentToken(TokenKind::Comma));
        consumeAndSkipBlanks();

        uintmax_t prio;
        TRY(parseUnsignedInteger(&prio));

        skipBlanks();
        TRY(expectCurrentToken(TokenKind::Comma));
        consumeAndSkipBlanks();

        TRY(expectCurrentToken(TokenKind::Identifier));

        bool isEnabled;
        if (_currentToken.value() == "false") {
            isEnabled = false;
        } else if (_currentToken.value() == "true") {
            isEnabled = true;
        } else {
            reportCurrentTokenError("expected 'true' or 'false'");
            return false;
        }

        consumeAndSkipBlanks();
        TRY(expectCurrentToken(TokenKind::RBracket));

        StatusMsg* msg = new StatusMsg(num, prio, isEnabled);
        bool isOk = comp->addStatus(num, msg);
        if (!isOk) {
            std::string msg =  "status with id " + std::to_string(num) + " already defined";
            reportTokenError(&numToken, msg.c_str());
            return false;
        }
        consumeAndSkipBlanks();
        TRY(expectCurrentToken(TokenKind::Colon));
        consumeAndSkipBlanks();
        auto parseOneRegexp = [this, comp](StatusMsg* msg) -> bool {
            TRY(expectCurrentToken(TokenKind::Identifier, "regular expression must begin with an identifier"));
            Rc<StatusRegexp> re = new StatusRegexp;

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
    clearUnusedDocComments();
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
        case TokenKind::Parameters: {
            TRY(parseParameters(comp.get()));
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
        case TokenKind::Impl: {
            TRY(parseComponentImpl(comp.get()));
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
    _fileInfo = finfo;

    _lastLineStart = _fileInfo->contents().c_str();
    _lexer = new Lexer(bmcl::StringView(_fileInfo->contents()));
    _ast = new Ast;

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




