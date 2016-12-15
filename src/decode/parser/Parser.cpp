#include "decode/parser/Parser.h"

#include "decode/core/FileInfo.h"
#include "decode/core/StringSwitch.h"
#include "decode/core/Diagnostics.h"
#include "decode/core/Try.h"
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
    endDecl(decl);
}

template <typename T>
void Parser::endDecl(const Rc<T>& decl)
{
    decl->_endLoc = _currentToken.location();
    decl->_end = _currentToken.begin();
    decl->_moduleInfo = _moduleInfo;
}

template <typename T>
Rc<T> Parser::beginType()
{
    Rc<TypeDecl> decl = beginDecl<TypeDecl>();
    Rc<T> type = new T;
    type->_moduleInfo = _moduleInfo;
    decl->_type = type;
    _ast->_typeToDecl.emplace(type, decl);
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
            Rc<ImportedType> type = beginType<ImportedType>();
            type->_importPath = importDecl->_importPath;
            type->_name = _currentToken.value();
            importDecl->_types.push_back(type);
            //TODO: check import conflicts
            _ast->_typeNameToType.emplace(type->_name, type);
            consumeAndEndType(type);
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
            case TokenKind::Impl:
                TRY(parseImplBlock());
                break;
            //case TokenKind::Eol:
            //    return true;
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

Rc<Function> Parser::parseFunction(bool selfAllowed)
{
    TRY(expectCurrentToken(TokenKind::Fn));
    Rc<Function> fn = beginType<Function>();
    consumeAndSkipBlanks();

    TRY(expectCurrentToken(TokenKind::Identifier));
    fn->_name = _currentToken.value();
    consume();

    TRY(parseList(TokenKind::LParen, TokenKind::Comma, TokenKind::RParen, fn, [this, &selfAllowed](const Rc<Function>& func) -> bool {
        if (selfAllowed) {
            if (currentTokenIs(TokenKind::Ampersand)) {
                consumeAndSkipBlanks();

                bool isMut = false;
                if (currentTokenIs(TokenKind::Mut)) {
                    isMut = true;
                    consumeAndSkipBlanks();
                }
                if (currentTokenIs(TokenKind::Self)) {
                    if (isMut) {
                        func->_self.emplace(SelfArgument::MutReference);
                    } else {
                        func->_self.emplace(SelfArgument::Reference);
                    }
                    consume();
                    selfAllowed = false;
                    return true;
                } else {
                    //TODO: report error
                    return false;
                }
            }

            if (currentTokenIs(TokenKind::Self)) {
                func->_self.emplace(SelfArgument::Value);
                consume();
                selfAllowed = false;
                return true;
            }
        }

        TRY(expectCurrentToken(TokenKind::Identifier));
        Rc<Field> field = beginDecl<Field>();
        field->_name = _currentToken.value();

        consumeAndSkipBlanks();

        TRY(expectCurrentToken(TokenKind::Colon));

        consumeAndSkipBlanks();

        Rc<Type> type = parseType();
        if (!type) {
            return false;
        }

        field->_type = type;

        endDecl(field);

        func->_arguments.push_back(field);
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

        fn->_returnValue.emplace(rType);
    }

    endType(fn);

    return fn;
}

bool Parser::parseImplBlock()
{
    TRY(skipCommentsAndSpace());
    TRY(expectCurrentToken(TokenKind::Impl));

    Rc<ImplBlock> block = beginDecl<ImplBlock>();
    consumeAndSkipBlanks();

    TRY(expectCurrentToken(TokenKind::Identifier));

    block->_name = _currentToken.value();
    consumeAndSkipBlanks();

    TRY(parseList(TokenKind::LBrace, TokenKind::Eol, TokenKind::RBrace, block, [this](const Rc<ImplBlock>& block) -> bool {
        Rc<Function> fn = parseFunction();
        if (!fn) {
            return false;
        }
        block->_funcs.push_back(fn);
        return true;
    }));

    bmcl::Option<const Rc<Type>&> type = _ast->findTypeWithName(block->name());
    if (type.isNone()) {
        //TODO: report error
        BMCL_CRITICAL() << "No type found for impl block";
        return false;
    }

    _ast->addImplBlock(block);

    return true;
}

Rc<Type> Parser::parseReferenceOrSliceType()
{
    TRY(skipCommentsAndSpace());
    TRY(expectCurrentToken(TokenKind::Ampersand));

    Rc<ReferenceType> type = beginType<ReferenceType>();
    consume();

    if (_currentToken.kind() == TokenKind::Mut) {
        type->_isMutable = true;
        consumeAndEndType(type);
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

    Rc<ReferenceType> type = beginType<ReferenceType>();
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
    consumeAndEndType(type);

    TRY(skipCommentsAndSpace());
    Rc<Type> pointee;
    if (_currentToken.kind() == TokenKind::Star) {
        pointee = parsePointerType();
    } else {
        pointee = parseBuiltinOrResolveType();
    }

    if (pointee) {
        type->_pointee = std::move(pointee);
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
        return parseReferenceOrSliceType();
    case TokenKind::LBracket:
        return parseArrayType();
    case TokenKind::Identifier:
        return parseBuiltinOrResolveType();
    default:
        //TODO: report error
        return nullptr;
    }

    return nullptr;
}

Rc<Type> Parser::parseFunctionPointer()
{
    TRY(expectCurrentToken(TokenKind::Ampersand));

    Rc<Function> fn = beginType<Function>();

    consume();
    TRY(expectCurrentToken(TokenKind::UpperFn));
    consume();

    TRY(parseList(TokenKind::LParen, TokenKind::Comma, TokenKind::RParen, fn, [this](const Rc<Function>& fn) {

        Rc<Type> type = parseType();
        if (!type) {
            return false;
        }
        Rc<Field> field = new Field;
        field->_moduleInfo = _moduleInfo;
        field->_type = type;
        fn->_arguments.push_back(field);

        return true;
    }));

    skipBlanks();

    if(currentTokenIs(TokenKind::RightArrow)) {
        consumeAndSkipBlanks();
        Rc<Type> rType = parseType();
        if (!rType) {
            return nullptr;
        }

        fn->_returnValue.emplace(rType);
    }

    endType(fn);
    return fn;
}

Rc<Type> Parser::parseSliceType()
{
    BMCL_CRITICAL() << "slices not supported yet";
    return nullptr;
    TRY(expectCurrentToken(TokenKind::Ampersand));
    Rc<SliceType> ref = beginType<SliceType>();
    consume();
    TRY(expectCurrentToken(TokenKind::LBracket));
    consumeAndSkipBlanks();

    Rc<Type> innerType = parseType();
    if (!innerType) {
        return nullptr;
    }

    ref->_elementType = innerType;

    consumeAndEndType(ref);
    return ref;
}

Rc<Type> Parser::parseArrayType()
{
    TRY(skipCommentsAndSpace());
    TRY(expectCurrentToken(TokenKind::LBracket));
    Rc<ArrayType> arrayType = beginType<ArrayType>();
    consumeAndSkipBlanks();

    Rc<Type> innerType = parseType();
    if (!innerType) {
        return nullptr;
    }

    skipBlanks();

    arrayType->_elementType = std::move(innerType);
    TRY(expectCurrentToken(TokenKind::SemiColon));
    consumeAndSkipBlanks();
    TRY(expectCurrentToken(TokenKind::Number));
    TRY(parseUnsignedInteger(&arrayType->_elementCount));
    skipBlanks();
    TRY(expectCurrentToken(TokenKind::RBracket));
    consumeAndEndType(arrayType);

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
        .Case("void", BuiltinTypeKind::Void)
        .Default(BuiltinTypeKind::Unknown);

    if (kind == BuiltinTypeKind::Unknown) {
        Rc<Type> link = findDeclaredType(_currentToken.value());
        if (!link) {
            //TODO: report error
            return nullptr;
        }
        consume();
        return link;
    }

    Rc<BuiltinType> type = beginType<BuiltinType>();
    type->_builtinTypeKind = kind;
    consumeAndEndType(type);
    return type;
}

Rc<Field> Parser::parseField()
{
    Rc<Field> decl = beginDecl<Field>();
    expectCurrentToken(TokenKind::Identifier);
    decl->_name = _currentToken.value();
    consumeAndSkipBlanks();
    expectCurrentToken(TokenKind::Colon);
    consumeAndSkipBlanks();

    Rc<Type> type = parseType();
    if (!type) {
        return nullptr;
    }
    decl->_type = type;

    endDecl(decl);
    return decl;
}

bool Parser::parseRecordField(const Rc<FieldList>& parent)
{
    Rc<Field> decl = parseField();
    if (!decl) {
        return false;
    }
    parent->push_back(decl);
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
        if (!parent->addConstant(constant)) {
            BMCL_CRITICAL() << "enum constant redefinition";
            return false;
        }
    } else {
        BMCL_CRITICAL() << "enum values MUST be user set";
        return false;
    }

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
        TRY(parseBraceList(field->_fields, std::bind(&Parser::parseRecordField, this, std::placeholders::_1)));
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
bool Parser::parseList(TokenKind openToken, TokenKind sep, TokenKind closeToken, T&& decl, F&& fieldParser)
{
    TRY(expectCurrentToken(openToken));
    consume();

    TRY(skipCommentsAndSpace());

    while (true) {
        if (_currentToken.kind() == closeToken) {
            consume();
            break;
        }

        if (!fieldParser(std::forward<T>(decl))) {
            return false;
        }

        skipBlanks();
        if (_currentToken.kind() == sep) {
            consume();
        }
        skipCommentsAndSpace();
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
    Rc<T> type = beginType<T>();
    TRY(expectCurrentToken(startToken));

    consume();
    TRY(skipCommentsAndSpace());
    TRY(expectCurrentToken(TokenKind::Identifier));
    type->_name = _currentToken.value();
    consumeAndSkipBlanks();

    TRY(parseBraceList(type, std::forward<F>(fieldParser)));

    _ast->_typeNameToType.emplace(type->name(), type);
    endType(type);
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
    return parseTag<StructType, true>(TokenKind::Struct, [this](const Rc<StructType>& decl) {
        if (!decl->_fields) {
            decl->_fields = new FieldList; //HACK
        }
        if (!parseRecordField(decl->_fields)) {
            return false;
        }
        return true;
    });
}

template<typename T, typename F>
Rc<T> Parser::parseNamelessTag(TokenKind startToken, TokenKind sep, F&& fieldParser)
{
    TRY(expectCurrentToken(startToken));
    Rc<T> decl = beginDecl<T>();
    consumeAndSkipBlanks();

    TRY(parseList(TokenKind::LBrace, sep, TokenKind::RBrace, decl, std::forward<F>(fieldParser)));

    endDecl(decl);
    return decl;
}

bool Parser::parseCommands(const Rc<Component>& parent)
{
    if(parent->_cmds) {
        reportCurrentTokenError("Component can have only one commands declaration");
        return false;
    }
    Rc<Commands> cmds = parseNamelessTag<Commands>(TokenKind::Commands, TokenKind::Eol, [this](const Rc<Commands>& cmds) {
        Rc<Function> fn = parseFunction(false);
        if (!fn) {
            return false;
        }
        cmds->_functions.push_back(fn);
        return true;
    });
    if (!cmds) {
        //TODO: report error
        return false;
    }
    parent->_cmds = cmds;
    return true;
}

bool Parser::parseParameters(const Rc<Component>& parent)
{
    if(parent->_params) {
        reportCurrentTokenError("Component can have only one parameters declaration");
        return false;
    }
    Rc<Parameters> params = parseNamelessTag<Parameters>(TokenKind::Parameters, TokenKind::Comma, [this](const Rc<Parameters>& params) {
        Rc<Field> field = parseField();
        if (!field) {
            return false;
        }
        params->_fields->push_back(field);
        return true;
    });
    if (!params) {
        //TODO: report error
        return false;
    }
    parent->_params = params;
    return true;
}

bool Parser::parseStatuses(const Rc<Component>& parent)
{
    if(parent->_statuses) {
        reportCurrentTokenError("Component can have only one statuses declaration");
        return false;
    }
    Rc<Statuses> statuses = parseNamelessTag<Statuses>(TokenKind::Statuses, TokenKind::Comma, [this, parent](const Rc<Statuses>& statuses) -> bool {
        uintmax_t n;
        TRY(parseUnsignedInteger(&n));
        auto pair = statuses->_regexps.emplace(std::piecewise_construct, std::forward_as_tuple(n), std::forward_as_tuple());
        if (!pair.second) {
            BMCL_CRITICAL() << "redefinition of status param";
            return false;
        }
        skipBlanks();
        TRY(expectCurrentToken(TokenKind::Colon));
        consumeAndSkipBlanks();
        auto parseOneRegexp = [this, parent](std::vector<Rc<StatusRegexp>>* regexps) -> bool {
            if (!currentTokenIs(TokenKind::Identifier)) {
                //TODO: report error
                return false;
            }
            Rc<StatusRegexp> re = new StatusRegexp;
            regexps->push_back(re);
//             Rc<FieldList> fields = parent->parameters()->fields();
//             Rc<Type> lastType;
//             Rc<Field> lastField;

            while (true) {
                if (currentTokenIs(TokenKind::Identifier)) {
//                     bmcl::Option<const Rc<Field>&> field  = fields->fieldWithName(_currentToken.value());
//                     if (field.isNone()) {
//                         //TODO: report error
//                         return false;
//                     }
                    Rc<FieldAccessor> acc = new FieldAccessor;
//                     acc->_field = field.unwrap();
//                     lastField = field.unwrap();
//                     lastType = field.unwrap()->type();
                    re->_accessors.push_back(acc);
                    consumeAndSkipBlanks();
                } else if (currentTokenIs(TokenKind::LBracket)) {
                    Rc<SubscriptAccessor> acc = new SubscriptAccessor;
//                     acc->_type = lastType;
                    consume();
                    uintmax_t m;
                    if (currentTokenIs(TokenKind::Number) && _lexer->nextIs(TokenKind::RBracket)) {
                        TRY(parseUnsignedInteger(&m));
                        acc->_subscript = bmcl::Either<Range, uintmax_t>(m);
                    } else {
                        acc->_subscript = bmcl::Either<Range, uintmax_t>(bmcl::InPlaceSecond);
                        if (currentTokenIs(TokenKind::Number)) {
                            TRY(parseUnsignedInteger(&m));
                            acc->_subscript.unwrapFirst()._lowerBound = m;
                            consume();
                        }

                        TRY(expectCurrentToken(TokenKind::DoubleDot));
                        consume();

                        if (currentTokenIs(TokenKind::Number)) {
                            TRY(parseUnsignedInteger(&m));
                            acc->_subscript.unwrapFirst()._upperBound = m;
                            consume();
                        }
                    }

                    TRY(expectCurrentToken(TokenKind::RBracket));
                    consumeAndSkipBlanks();

                    re->_accessors.push_back(acc);

//                     if (lastType->isSlice()) {
//                         SliceType* slice = lastType->asSlice();
//                         lastType = slice->elementType();
//                     } else if (lastType->isArray()) {
//                         ArrayType* array = lastType->asArray();
//                         lastType = array->elementType();
//                         //TODO: check ranges
//                     } else {
//                         //TODO: report error
//                         return false;
//                     }
                }
                if (currentTokenIs(TokenKind::Comma) || currentTokenIs(TokenKind::RBrace)) {
                    return true;
                } else if (currentTokenIs(TokenKind::Dot)) {
//                     if (!lastType->isStruct()) {
//                         //TODO: report error
//                         return false;
//                     }
//                     fields = lastType->asStruct()->fields();
                    consume();
                }
            }
            return true;
        };
        std::vector<Rc<StatusRegexp>>& regexps = pair.first->second;
        if (currentTokenIs(TokenKind::LBrace)) {
            TRY(parseBraceList(&regexps, parseOneRegexp));
        } else if (currentTokenIs(TokenKind::Identifier)) {
            TRY(parseOneRegexp(&regexps));
        }
        return true;
    });
    if (!statuses) {
        //TODO: report error
        return false;
    }
    parent->_statuses = statuses;
    return true;
}

bool Parser::parseComponent()
{
    TRY(expectCurrentToken(TokenKind::Component));
    Rc<Component> comp = new Component;
    comp->_moduleName = _moduleInfo->moduleName();
    consumeAndSkipBlanks();
    TRY(expectCurrentToken(TokenKind::Identifier));
    //_ast->addTopLevelType(comp);
    consumeAndSkipBlanks();

    TRY(expectCurrentToken(TokenKind::LBrace));
    consume();

    while(true) {
        TRY(skipCommentsAndSpace());

        switch(_currentToken.kind()) {
        case TokenKind::Parameters: {
            TRY(parseParameters(comp));
            break;
        }
        case TokenKind::Commands: {
            TRY(parseCommands(comp));
            break;
        }
        case TokenKind::Statuses: {
            TRY(parseStatuses(comp));
            break;
        }
        case TokenKind::RBrace:
            consume();
            goto finish;
        default:
            //TODO: report error
            return false;
        }
    }

finish:
    return true;
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
