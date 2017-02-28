#include "decode/parser/Parser.h"

#include "decode/core/FileInfo.h"
#include "decode/core/StringSwitch.h"
#include "decode/core/Diagnostics.h"
#include "decode/core/CfgOption.h"
#include "decode/parser/Decl.h"
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

Parser::Parser(const Rc<Diagnostics>& diag)
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
    ADD_BUILTIN(boolType, Bool, "bool");
    ADD_BUILTIN(voidType, Void, "void");
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

ParseResult Parser::parseFile(const char* fname)
{
    bmcl::Result<std::string, int> rv = bmcl::readFileIntoString(fname);
    if (rv.isErr()) {
        //TODO: report error
        return ParseResult();
    }

    Rc<FileInfo> finfo = new FileInfo(std::string(fname), rv.take());
    return parseFile(finfo);

}

ParseResult Parser::parseFile(const Rc<FileInfo>& finfo)
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
    decl->_type = type;
    _ast->_typeToDecl.emplace(type, decl);
    _typeDeclStack.push_back(decl);
    return type;
}

template <typename T>
Rc<T> Parser::beginNamedType()
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
        Rc<TypeImport> importDecl = beginDecl<TypeImport>();
        TRY(consumeAndExpectCurrentToken(TokenKind::Blank));
        TRY(consumeAndExpectCurrentToken(TokenKind::Identifier));
        importDecl->_importPath = _currentToken.value();
        TRY(consumeAndExpectCurrentToken(TokenKind::DoubleColon));
        consume();

        auto createImportedTypeFromCurrentToken = [this, importDecl]() {
            Rc<ImportedType> type = beginNamedType<ImportedType>();
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
                BMCL_CRITICAL() << "Unexpected top level decl token";
                //TODO: report error
                return false;
        }
    }
    return true;
}

bool Parser::parseConstant()
{
    TRY(expectCurrentToken(TokenKind::Const));
    Rc<Constant> constant = new Constant;
    consumeAndSkipBlanks();

    TRY(expectCurrentToken(TokenKind::Identifier));
    constant->_name = _currentToken.value();
    consumeAndSkipBlanks();

    TRY(expectCurrentToken(TokenKind::Colon));
    consumeAndSkipBlanks();

    Rc<Type> type = parseBuiltinOrResolveType();
    if (!type) {
        return false;
    }
    if (type->typeKind() != TypeKind::Builtin) {
        BMCL_CRITICAL() << "Constant can only be of builtin type";
        return false;
    }
    constant->_type = type;

    skipBlanks();
    TRY(expectCurrentToken(TokenKind::Equality));
    consumeAndSkipBlanks();

    std::uintmax_t value;
    TRY(parseUnsignedInteger(&value));
    constant->_value = value;

    TRY(expectCurrentToken(TokenKind::SemiColon));
    consume();

    _ast->_constants.emplace(constant->name(), constant);

    return true;
}

bool Parser::parseAttribute()
{
    TRY(skipCommentsAndSpace());
    TRY(expectCurrentToken(TokenKind::Hash));
    consume();

    TRY(expectCurrentToken(TokenKind::LBracket));
    consumeAndSkipBlanks();

    TRY(expectCurrentToken(TokenKind::Identifier));

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
        BMCL_CRITICAL() << "Only cfg attributes supported";
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
            opt->addOption(nopt);
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

Rc<FunctionType> Parser::parseFunction(bool selfAllowed)
{
    TRY(expectCurrentToken(TokenKind::Fn));
    Rc<FunctionType> fn = beginNamedType<FunctionType>();
    consumeAndSkipBlanks();

    TRY(expectCurrentToken(TokenKind::Identifier));
    fn->_name = _currentToken.value();
    consume();

    TRY(parseList(TokenKind::LParen, TokenKind::Comma, TokenKind::RParen, fn, [this, &selfAllowed](const Rc<FunctionType>& func) -> bool {
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
        Rc<Field> field = new Field;
        field->_name = _currentToken.value();

        consumeAndSkipBlanks();

        TRY(expectCurrentToken(TokenKind::Colon));

        consumeAndSkipBlanks();

        Rc<Type> type = parseType();
        if (!type) {
            return false;
        }

        field->_type = type;

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
        Rc<FunctionType> fn = parseFunction();
        if (!fn) {
            return false;
        }
        block->_funcs.push_back(fn);
        return true;
    }));

    bmcl::Option<const Rc<NamedType>&> type = _ast->findTypeWithName(block->name());
    if (type.isNone()) {
        //TODO: report error
        BMCL_CRITICAL() << "No type found for impl block";
        return false;
    }

    _ast->addImplBlock(block);

    return true;
}

bool Parser::parseAlias()
{
    TRY(skipCommentsAndSpace());
    TRY(expectCurrentToken(TokenKind::Type));

    Rc<AliasType> type = beginType<AliasType>();
    consumeAndSkipBlanks();
    TRY(expectCurrentToken(TokenKind::Identifier));
    type->_name = _currentToken.value();

    consumeAndSkipBlanks();
    TRY(expectCurrentToken(TokenKind::Equality));

    consumeAndSkipBlanks();

    Rc<Type> link = parseType();
    if (!type) {
        return false;
    }

    type->_alias = link;
    type->_moduleInfo = _moduleInfo;
    skipBlanks();

    TRY(expectCurrentToken(TokenKind::SemiColon));
    consume();

    _ast->addTopLevelType(type);

    return true;
}

Rc<Type> Parser::parseReferenceType()
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
        return parseReferenceType();
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

    Rc<FunctionType> fn = beginNamedType<FunctionType>();

    consume();
    TRY(expectCurrentToken(TokenKind::UpperFn));
    consume();

    TRY(parseList(TokenKind::LParen, TokenKind::Comma, TokenKind::RParen, fn, [this](const Rc<FunctionType>& fn) {

        Rc<Type> type = parseType();
        if (!type) {
            return false;
        }
        Rc<Field> field = new Field;
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
    TRY(expectCurrentToken(TokenKind::Ampersand));
    Rc<SliceType> ref = beginType<SliceType>();
    ref->_moduleInfo = _moduleInfo;
    consume();
    TRY(expectCurrentToken(TokenKind::LBracket));
    consumeAndSkipBlanks();

    Rc<Type> innerType = parseType();
    if (!innerType) {
        return nullptr;
    }

    ref->_elementType = innerType;

    TRY(expectCurrentToken(TokenKind::RBracket));
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
         BMCL_CRITICAL() << "invalid number";
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
    auto it = _builtinTypes->btMap.find(_currentToken.value());
    if (it != _builtinTypes->btMap.end()) {
        consume();
        return it->second;
    }
    Rc<Type> link = findDeclaredType(_currentToken.value());
    if (!link) {
        //TODO: report error
        BMCL_CRITICAL() << "unknown type " << _currentToken.value().toStdString();
        return nullptr;
    }
    consume();
    return link;
}

Rc<Field> Parser::parseField()
{
    Rc<Field> field = new Field;
    expectCurrentToken(TokenKind::Identifier);
    field->_name = _currentToken.value();
    consumeAndSkipBlanks();
    expectCurrentToken(TokenKind::Colon);
    consumeAndSkipBlanks();

    Rc<Type> type = parseType();
    if (!type) {
        return nullptr;
    }
    field->_type = type;

    return field;
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

bool Parser::parseEnumConstant(const Rc<EnumType>& parent)
{
    TRY(skipCommentsAndSpace());
    Rc<EnumConstant> constant = new EnumConstant;
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

    return true;
}

bool Parser::parseVariantField(const Rc<VariantType>& parent)
{
    TRY(expectCurrentToken(TokenKind::Identifier));
    bmcl::StringView name = _currentToken.value();
    consumeAndSkipBlanks();
     //TODO: peek next token

    if (currentTokenIs(TokenKind::Comma)) {
        Rc<ConstantVariantField> field = new ConstantVariantField;
        field->_name = name;
        parent->_fields.push_back(field);
    } else if (currentTokenIs(TokenKind::LBrace)) {
        Rc<StructVariantField> field = new StructVariantField;
        field->_fields = new FieldList; //HACK
        field->_name = name;
        TRY(parseBraceList(field->_fields, std::bind(&Parser::parseRecordField, this, std::placeholders::_1)));
        parent->_fields.push_back(field);
    } else if (currentTokenIs(TokenKind::LParen)) {
        Rc<TupleVariantField> field = new TupleVariantField;
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
    Rc<T> type = beginNamedType<T>();
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
    return parseTag<VariantType, true>(TokenKind::Variant, std::bind(&Parser::parseVariantField, this, std::placeholders::_1));
}

bool Parser::parseEnum()
{
    return parseTag<EnumType, false>(TokenKind::Enum, std::bind(&Parser::parseEnumConstant, this, std::placeholders::_1));
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
    Rc<T> decl = new T;
    consumeAndSkipBlanks();

    TRY(parseList(TokenKind::LBrace, sep, TokenKind::RBrace, decl, std::forward<F>(fieldParser)));

    return decl;
}

bool Parser::parseCommands(const Rc<Component>& parent)
{
    if(parent->_cmds.isSome()) {
        reportCurrentTokenError("Component can have only one commands declaration");
        return false;
    }
    Rc<Commands> cmds = parseNamelessTag<Commands>(TokenKind::Commands, TokenKind::Eol, [this](const Rc<Commands>& cmds) {
        Rc<FunctionType> fn = parseFunction(false);
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

bool Parser::parseComponentImpl(const Rc<Component>& parent)
{
    if(parent->_implBlock.isSome()) {
        reportCurrentTokenError("Component can have only one impl declaration");
        return false;
    }
    Rc<ImplBlock> impl = parseNamelessTag<ImplBlock>(TokenKind::Impl, TokenKind::Eol, [this](const Rc<ImplBlock>& impl) {
        Rc<FunctionType> fn = parseFunction(false);
        if (!fn) {
            return false;
        }
        impl->_funcs.push_back(fn);
        return true;
    });
    if (!impl) {
        //TODO: report error
        return false;
    }
    parent->_implBlock = impl;
    return true;
}

bool Parser::parseParameters(const Rc<Component>& parent)
{
    if(parent->_params.isSome()) {
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
    if(parent->_statuses.isSome()) {
        reportCurrentTokenError("Component can have only one statuses declaration");
        return false;
    }
    Rc<Statuses> statuses = parseNamelessTag<Statuses>(TokenKind::Statuses, TokenKind::Comma, [this, parent](const Rc<Statuses>& statuses) -> bool {
        uintmax_t n;
        TRY(parseUnsignedInteger(&n));
        StatusMsg* msg = new StatusMsg(n);
        auto pair = statuses->_statusMap.emplace(std::piecewise_construct, std::forward_as_tuple(n), std::forward_as_tuple(msg));
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

            while (true) {
                if (currentTokenIs(TokenKind::Identifier)) {
                    Rc<FieldAccessor> acc = new FieldAccessor;
                    acc->_value = _currentToken.value();
                    re->_accessors.push_back(acc);
                    consumeAndSkipBlanks();
                } else if (currentTokenIs(TokenKind::LBracket)) {
                    Rc<SubscriptAccessor> acc = new SubscriptAccessor;
                    consume();
                    uintmax_t m;
                    if (currentTokenIs(TokenKind::Number) && _lexer->nextIs(TokenKind::RBracket)) {
                        TRY(parseUnsignedInteger(&m));
                        acc->_subscript = bmcl::Either<Range, uintmax_t>(bmcl::InPlaceSecond, m);
                    } else {
                        acc->_subscript = bmcl::Either<Range, uintmax_t>(bmcl::InPlaceFirst);
                        if (currentTokenIs(TokenKind::Number)) {
                            TRY(parseUnsignedInteger(&m));
                            acc->_subscript.unwrapFirst().lowerBound.emplace(m);
                        }

                        TRY(expectCurrentToken(TokenKind::DoubleDot));
                        consume();

                        if (currentTokenIs(TokenKind::Number)) {
                            TRY(parseUnsignedInteger(&m));
                            acc->_subscript.unwrapFirst().upperBound.emplace(m);
                        }
                    }

                    TRY(expectCurrentToken(TokenKind::RBracket));
                    consumeAndSkipBlanks();

                    re->_accessors.push_back(acc);


                }
                skipCommentsAndSpace();
                if (currentTokenIs(TokenKind::Comma) || currentTokenIs(TokenKind::RBrace)) {
                    break;
                } else if (currentTokenIs(TokenKind::Dot)) {
                    consume();
                }
            }
            if (!re->_accessors.empty()) {
                regexps->push_back(re);
            }
            return true;
        };
        std::vector<Rc<StatusRegexp>>& parts = pair.first->second->_parts;
        if (currentTokenIs(TokenKind::LBrace)) {
            TRY(parseBraceList(&parts, parseOneRegexp));
        } else if (currentTokenIs(TokenKind::Identifier)) {
            TRY(parseOneRegexp(&parts));
        }
        return true;
    });
    if (!statuses) {
        //TODO: report error
        return false;
    }
    if (!statuses->statusMap().empty()) {
        parent->_statuses = statuses;
    }
    return true;
}

//TODO: make component numbers explicit

bool Parser::parseComponent()
{
    TRY(expectCurrentToken(TokenKind::Component));
    Rc<Component> comp = new Component;
    comp->_moduleName = _moduleInfo->moduleName();
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
        case TokenKind::Impl: {
            TRY(parseComponentImpl(comp));
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
    //TODO: only one component allowed, add check
    _ast->_component = comp;
    return true;
}

bool Parser::parseOneFile(const Rc<FileInfo>& finfo)
{
    _fileInfo = finfo;

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
