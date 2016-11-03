#include "decode/parser/Parser.h"

#include "decode/core/FileInfo.h"
#include "decode/core/StringSwitch.h"
#include "decode/core/Diagnostics.h"
#include "decode/parser/Decl.h"
#include "decode/parser/Ast.h"
#include "decode/parser/Token.h"
#include "decode/parser/Decl.h"
#include "decode/parser/Type.h"
#include "decode/parser/Lexer.h"
#include "decode/parser/ModuleInfo.h"

#include <bmcl/FileUtils.h>
#include <bmcl/Logging.h>
#include <bmcl/Result.h>

#include <string>
#include <functional>

#define TRY(expr) \
    do { \
        if (!expr) { \
            return 0; \
        } \
    } while(0);

namespace decode {

Parser::Parser(const Rc<Diagnostics>& diag)
    : _diag(diag)
{
}

Parser::~Parser()
{
}

void Parser::finishSplittingLines()
{
    const char* start = _lastLineStart;
    const char* current = start;
    while (true) {
        char c = *current;
        if (c == '\n') {
            _fileInfo->_lines.emplace_back(start, current);
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
    reportCurrentTokenError("Unexpected token");
}

ParseResult<Rc<Ast>> Parser::parseFile(const char* fname)
{
    cleanup();
    if (parseOneFile(fname)) {
        finishSplittingLines();
        return _ast;
    }
    finishSplittingLines();
    return ParseResult<Rc<Ast>>();
}

void Parser::consume()
{
    _lexer->consumeNextToken(&_currentToken);
}

void Parser::addLine()
{
    const char* current = _currentToken.begin();
    _fileInfo->_lines.emplace_back(_lastLineStart, current);
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
        case TokenKind::RawComment:
            consume();
            break;
        case TokenKind::DocComment:
            consume();
            break;
        case TokenKind::Eof:
            addLine();
            return true;
        case TokenKind::Invalid:
            reportUnexpectedTokenError(TokenKind::Invalid);
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

bool Parser::consumeAndExpectCurrentToken(TokenKind expected)
{
    _lexer->consumeNextToken(&_currentToken);
    return expectCurrentToken(expected);
}

bool Parser::expectCurrentToken(TokenKind expected)
{
    return expectCurrentToken(expected, "Unexpected token");
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
    decl->_startLoc = _currentToken.location();
    decl->_start = _currentToken.begin();
    return decl;
}

template <typename T>
void Parser::consumeAndEndDecl(const Rc<T>& decl)
{
    _lexer->consumeNextToken(&_currentToken);
    decl->_endLoc = _currentToken.location();
    decl->_end = _currentToken.begin();
    decl->_moduleInfo = _moduleInfo;
}

template <typename T>
void Parser::endDecl(const Rc<T>& decl)
{
    decl->_endLoc = _currentToken.location();
    decl->_end = _currentToken.begin();
    decl->_moduleInfo = _moduleInfo;
}

bool Parser::consumeAndSetNamedDeclName(NamedDecl* decl)
{
    TRY(consumeAndExpectCurrentToken(TokenKind::Identifier));
    decl->_name = _currentToken.value();
    return true;
}

bool Parser::currentTokenIs(TokenKind kind)
{
    return _currentToken.kind() == kind;
}

Rc<Report> Parser::reportCurrentTokenError(const char* msg)
{
    Rc<Report> report = _diag->addReport(_fileInfo);
    report->setLocation(_currentToken.location());
    report->setLevel(Report::Error);
    report->setMessage(msg);
    return report;
}

bool Parser::parseModuleDecl()
{
    TRY(skipCommentsAndSpace());
    _moduleInfo = new ModuleInfo;
    _moduleInfo->_fileInfo = _fileInfo;
    _ast->_moduleInfo = _moduleInfo;
    Rc<Module> modDecl = beginDecl<Module>();

    TRY(expectCurrentToken(TokenKind::Module, "Every module must begin with module declaration"));
    consume();

    TRY(expectCurrentToken(TokenKind::Blank, "Missing blanks after module keyword"));
    consume();

    TRY(expectCurrentToken(TokenKind::Identifier));

    modDecl->_name = _currentToken.value();
    _moduleInfo->_moduleName = modDecl->_name;
    consumeAndEndDecl(modDecl);

    _ast->_moduleDecl = std::move(modDecl);

    return true;
}

bool Parser::parseImports()
{
    while (true) {
        TRY(skipCommentsAndSpace());
        if (_currentToken.kind() != TokenKind::Import) {
            return true;
        }
        Rc<Import> importDecl = beginDecl<Import>();
        TRY(consumeAndExpectCurrentToken(TokenKind::Blank));
        TRY(consumeAndExpectCurrentToken(TokenKind::Identifier));
        importDecl->_importPath = _currentToken.value();
        TRY(consumeAndExpectCurrentToken(TokenKind::DoubleColon));
        consume();

        auto createImportedTypeFromCurrentToken = [this, importDecl]() {
            Rc<ImportedType> type = beginDecl<ImportedType>();
            type->_importPath = importDecl->_importPath;
            type->_name = _currentToken.value();
            importDecl->_types.push_back(type);
            //TODO: check import conflicts
            _ast->_typeNameToType.emplace(type->_name, type);
            consumeAndEndDecl(type);
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
                    //TODO: report error
                    return false;
                }
            }
        } else {
            //TODO: report error
            return false;
        }

        endDecl(importDecl);
        _ast->_importDecls.push_back(std::move(importDecl));
    }

    return true;
}

bool Parser::parseTopLevelDecls()
{
    while (true) {
        TRY(skipCommentsAndSpace());

        switch (_currentToken.kind()) {
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
            case TokenKind::Eol:
                return true;
            case TokenKind::Eof:
                return true;
            default:
                BMCL_CRITICAL() << "Unexpected top level decl token";
                //TODO: report error
                return false;
        }
    }
    return true;
}

Rc<Type> Parser::parseReferenceOrSliceType()
{
    TRY(skipCommentsAndSpace());
    TRY(expectCurrentToken(TokenKind::Ampersand));
    consume();

    Rc<ReferenceType> type = beginDecl<ReferenceType>();

    if (_currentToken.kind() == TokenKind::Mut) {
        type->_isMutable = true;
        consumeAndEndDecl(type);
    } else {
        type->_isMutable = false;
    }
    skipBlanks();

    Rc<Type> pointee;
    if (_currentToken.kind() == TokenKind::LBracket) {
        consumeAndSkipBlanks();
        pointee = parseType();

    } else if (_currentToken.kind() == TokenKind::Identifier) {
        pointee = parseBuiltinOrResolveType();
    } else {
        reportUnexpectedTokenError(_currentToken.kind());
        return nullptr;
    }

    if (!pointee) {
        return nullptr;
    }

    type->_pointee = std::move(pointee);
    return type;
}

Rc<Type> Parser::parsePointerType()
{
    TRY(skipCommentsAndSpace());
    TRY(expectCurrentToken(TokenKind::Star));
    consume();

    Rc<ReferenceType> type = beginDecl<ReferenceType>();
    type->_referenceKind = ReferenceKind::Pointer;

    if (_currentToken.kind() == TokenKind::Mut) {
        type->_isMutable = true;
    } else if (_currentToken.kind() == TokenKind::Const) {
        type->_isMutable = false;
    } else {
        //TODO: handle error
        reportUnexpectedTokenError(_currentToken.kind());
        return nullptr;
    }
    consumeAndEndDecl(type);

    TRY(skipCommentsAndSpace());
    Rc<Type> pointee;
    if (_currentToken.kind() == TokenKind::Star) {
        pointee = parsePointerType();
    } else {
        pointee = parseNonReferenceType(false);
    }

    if (pointee) {
        type->_pointee = std::move(pointee);
        return type;
    }

    return nullptr;
}

Rc<Type> Parser::parseNonReferenceType(bool sliceAllowed)
{
    TRY(skipCommentsAndSpace());

    switch (_currentToken.kind()) {
    case TokenKind::LBracket:
        return parseArrayType(sliceAllowed);
    case TokenKind::Identifier:
        return parseBuiltinOrResolveType();
    default:
        reportUnexpectedTokenError(_currentToken.kind());
        return nullptr;
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
        return parseReferenceOrSliceType();
    default:
        return parseNonReferenceType(true);
    }

    return nullptr;
}

Rc<Type> Parser::parseArrayType(bool sliceAllowed)
{
    TRY(skipCommentsAndSpace());

    TRY(expectCurrentToken(TokenKind::LBracket));
    Rc<Decl> decl = beginDecl<Decl>();
    consumeAndSkipBlanks();

    Rc<Type> innerType = parseType();
    if (!innerType) {
        return nullptr;
    }

    skipBlanks();

    if (currentTokenIs(TokenKind::RBracket)) {
        if (sliceAllowed) {
            Rc<ReferenceType> ref = new ReferenceType;
            decl->cloneDeclTo(ref.get());
            ref->_isMutable = false;
            ref->_pointee = innerType;
            ref->_referenceKind = ReferenceKind::Slice;
            consumeAndEndDecl(ref);
            return ref;
        } else {
            reportCurrentTokenError("Slices are not allowed in this context");
            return nullptr;
        }
    }

    Rc<ArrayType> arrayType = new ArrayType;
    decl->cloneDeclTo(arrayType.get());
    arrayType->_elementType = std::move(innerType);
    TRY(expectCurrentToken(TokenKind::SemiColon));
    consumeAndSkipBlanks();
    TRY(expectCurrentToken(TokenKind::Number));
    TRY(parseUnsignedInteger(&arrayType->_elementCount));
    skipBlanks();
    TRY(expectCurrentToken(TokenKind::RBracket));
    consumeAndEndDecl(arrayType);

    return arrayType;
}

bool Parser::parseUnsignedInteger(std::uintmax_t* dest)
{
    TRY(expectCurrentToken(TokenKind::Number));

    unsigned long long int value = std::strtoull(_currentToken.begin(), 0, 10);
    //TODO: check for target overflow
    if (errno == ERANGE) {
         errno = 0;
         // TODO: report range error
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
    }

    TRY(expectCurrentToken(TokenKind::Number));

    long long int value = std::strtoll(start, 0, 10);
    //TODO: check for target overflow
    if (errno == ERANGE) {
         errno = 0;
         // TODO: report range error
         return false;
    }
    *dest = value;
    consume();

    return true;
}

Rc<Type> Parser::findDeclaredType(bmcl::StringView name) const
{
    auto it = _ast->_typeNameToType.find(name);
    if (it == _ast->_typeNameToType.end()) {
        return nullptr;
    }
    return it->second;
}

Rc<Type> Parser::parseBuiltinOrResolveType()
{
    BuiltinTypeKind kind = StringSwitch<BuiltinTypeKind>(_currentToken.value())
        .Case("usize", BuiltinTypeKind::USize)
        .Case("isize", BuiltinTypeKind::ISize)
        .Case("varuint", BuiltinTypeKind::Varuint)
        .Case("varint", BuiltinTypeKind::Varint)
        .Case("u8", BuiltinTypeKind::U8)
        .Case("i8", BuiltinTypeKind::I8)
        .Case("u16", BuiltinTypeKind::U16)
        .Case("i16", BuiltinTypeKind::I16)
        .Case("u32", BuiltinTypeKind::U32)
        .Case("i32", BuiltinTypeKind::I32)
        .Case("u64", BuiltinTypeKind::U64)
        .Case("i64", BuiltinTypeKind::I64)
        .Case("bool", BuiltinTypeKind::Bool)
        .Default(BuiltinTypeKind::Unknown);

    if (kind == BuiltinTypeKind::Unknown) {
        Rc<ResolvedType> type = beginDecl<ResolvedType>();
        type->_name = _currentToken.value();
        Rc<Type> link = findDeclaredType(_currentToken.value());
        if (!link) {
            return nullptr;
        }
        type->_resolvedType = link;
        consumeAndEndDecl(type);
        return type;
    }

    Rc<BuiltinType> type = beginDecl<BuiltinType>();
    type->_builtinTypeKind = kind;
    consumeAndEndDecl(type);
    return type;
}

bool Parser::parseRecordField(const Rc<FieldList>& parent)
{
    Rc<Field> decl = beginDecl<Field>();
    expectCurrentToken(TokenKind::Identifier);
    decl->_name = _currentToken.value();
    consumeAndSkipBlanks();
    expectCurrentToken(TokenKind::Colon);
    consumeAndSkipBlanks();

    Rc<Type> type = parseType();
    if (!type) {
        return false;
    }
    decl->_type = type;

    endDecl(decl);
    parent->_fields.push_back(decl);
    parent->_fieldNameToFieldMap.emplace(decl->name(), decl);
    return true;
}

bool Parser::parseEnumConstant(const Rc<Enum>& parent)
{
    TRY(skipCommentsAndSpace());
    Rc<EnumConstant> constant = beginDecl<EnumConstant>();
    TRY(expectCurrentToken(TokenKind::Identifier));
    constant->_name = _currentToken.value();
    consumeAndSkipBlanks();

    if (currentTokenIs(TokenKind::Equality)) {
        consumeAndSkipBlanks();
        intmax_t value;
        TRY(parseSignedInteger(&value));
        //TODO: check value
        constant->_value = value;
        constant->_isUserSet = true;
    } else {
        constant->_isUserSet = false;
    }
    parent->_constantDecls.push_back(constant);

    endDecl(constant);
    return true;
}

bool Parser::parseVariantField(const Rc<Variant>& parent)
{
    TRY(expectCurrentToken(TokenKind::Identifier));
    bmcl::StringView name = _currentToken.value();
    consumeAndSkipBlanks();
     //TODO: peek next token

    if (currentTokenIs(TokenKind::Comma)) {
        Rc<ConstantVariantField> field = beginDecl<ConstantVariantField>();
        field->_name = name;
        endDecl(field);
        parent->_fields.push_back(field);
    } else if (currentTokenIs(TokenKind::LBrace)) {
        Rc<StructVariantField> field = beginDecl<StructVariantField>();
        field->_fields = new FieldList; //HACK
        field->_name = name;
        TRY(parseList(TokenKind::LBrace, TokenKind::Comma, TokenKind::RBrace,
                      field->_fields, std::bind(&Parser::parseRecordField, this, std::placeholders::_1)));
        endDecl(field);
        parent->_fields.push_back(field);
    } else if (currentTokenIs(TokenKind::LParen)) {
        Rc<TupleVariantField> field = beginDecl<TupleVariantField>();
        field->_name = name;
        TRY(parseList(TokenKind::LParen, TokenKind::Comma, TokenKind::RParen, field, [this](const Rc<TupleVariantField>& field) {
            skipBlanks();
            Rc<Type> type = parseType();
            if (!type) {
                return false;
            }
            field->_types.push_back(type);
            return true;
        }));
        endDecl(field);
        parent->_fields.push_back(field);
    } else {
        reportUnexpectedTokenError(_currentToken.kind());
        return false;
    }

    return true;
}

bool Parser::parseComponentField(const Rc<Component>& parent)
{
    return false;
}

template <typename T, typename F>
bool Parser::parseList(TokenKind openToken, TokenKind sep, TokenKind closeToken, const Rc<T>& decl, F&& fieldParser)
{
    TRY(expectCurrentToken(openToken));
    consume();

    skipCommentsAndSpace();
    TRY(skipCommentsAndSpace());

    while (true) {
        if (!fieldParser(decl)) {
            return false;
        }

        skipBlanks();
        if (_currentToken.kind() == sep) {
            consume();
        }
        skipCommentsAndSpace();

        if (_currentToken.kind() == closeToken) {
            consumeAndEndDecl(decl);
            break;
        }
    }

    return true;
}

template <typename T, bool genericAllowed, typename F>
bool Parser::parseTag(TokenKind startToken, F&& fieldParser)
{
    TRY(skipCommentsAndSpace());
    Rc<T> tagDecl = beginDecl<T>();
    TRY(expectCurrentToken(startToken));

    consume();
    TRY(skipCommentsAndSpace());
    TRY(expectCurrentToken(TokenKind::Identifier));
    tagDecl->_name = _currentToken.value();
    consumeAndSkipBlanks();

    TRY(parseList<T>(TokenKind::LBrace, TokenKind::Comma, TokenKind::RBrace, tagDecl, std::forward<F>(fieldParser)));

    _ast->_typeNameToType.emplace(tagDecl->name(), tagDecl);
    return true;
}

bool Parser::parseVariant()
{
    return parseTag<Variant, true>(TokenKind::Variant, std::bind(&Parser::parseVariantField, this, std::placeholders::_1));
}

bool Parser::parseEnum()
{
    return parseTag<Enum, false>(TokenKind::Enum, std::bind(&Parser::parseEnumConstant, this, std::placeholders::_1));
}

bool Parser::parseStruct()
{
    return parseTag<StructDecl, true>(TokenKind::Struct, [this](const Rc<StructDecl>& decl) {
        if (!decl->_fields) {
            decl->_fields = beginDecl<FieldList>(); //HACK
        }
        if (!parseRecordField(decl->_fields)) {
            return false;
        }
        endDecl(decl->_fields);
        return true;
    });
}

bool Parser::parseComponent()
{
    return parseTag<Component, false>(TokenKind::Component, std::bind(&Parser::parseComponentField, this, std::placeholders::_1));
}

Rc<Type> Parser::parseSliceType()
{
    return nullptr;
}

bool Parser::parseOneFile(const char* fname)
{
    bmcl::Result<std::string, int> rv = bmcl::readFileIntoString(fname);
    if (rv.isErr()) {
        return false;
    }

    _fileInfo = new FileInfo(std::string(fname), rv.take());
    _lastLineStart = _fileInfo->contents().c_str();
    _lexer = new Lexer(bmcl::StringView(_fileInfo->contents()));
    _ast = new Ast;

    _lexer->consumeNextToken(&_currentToken);
    TRY(parseModuleDecl());
    TRY(parseImports());
    TRY(parseTopLevelDecls());

    return true;
}

void Parser::cleanup()
{
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
