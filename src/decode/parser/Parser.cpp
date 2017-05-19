#include "decode/parser/Parser.h"

#include "decode/core/FileInfo.h"
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

Rc<Report> Parser::reportCurrentTokenError(const char* msg)
{
    Rc<Report> report = _diag->addReport(_fileInfo.get());
    report->setLocation(_currentToken.location());
    report->setLevel(Report::Error);
    report->setMessage(msg);
    return report;
}

bool Parser::parseModuleDecl()
{
    TRY(skipCommentsAndSpace());

    Location start = _currentToken.location();
    TRY(expectCurrentToken(TokenKind::Module, "Every module must begin with module declaration"));
    consume();

    TRY(expectCurrentToken(TokenKind::Blank, "Missing blanks after module keyword"));
    consume();

    TRY(expectCurrentToken(TokenKind::Identifier));

    bmcl::StringView modName = _currentToken.value();
    _moduleInfo = new ModuleInfo(modName, _fileInfo.get());

    Rc<ModuleDecl> modDecl = new ModuleDecl(_moduleInfo.get(), start, _currentToken.location());
    _ast->setModuleDecl(modDecl.get());
    consume();

    return true;
}

bool Parser::parseImports()
{
    while (true) {
        TRY(skipCommentsAndSpace());
        if (_currentToken.kind() != TokenKind::Import) {
            return true;
        }
        TRY(consumeAndExpectCurrentToken(TokenKind::Blank));
        TRY(consumeAndExpectCurrentToken(TokenKind::Identifier));
        Rc<TypeImport> import = new TypeImport(_currentToken.value());
        TRY(consumeAndExpectCurrentToken(TokenKind::DoubleColon));
        consume();

        auto createImportedTypeFromCurrentToken = [this, import]() {
            Rc<ImportedType> type = new ImportedType(_currentToken.value(), import->path(), _moduleInfo.get());
            import->addType(type.get());
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
                    //TODO: report error
                    return false;
                }
            }
        } else {
            //TODO: report error
            return false;
        }

        _ast->addImportDecl(import.get());
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
                BMCL_CRITICAL() << "Unexpected top level decl token" << _currentToken.line();
                //TODO: report error
                return false;
        }
    }
    return true;
}

bool Parser::parseConstant()
{
    TRY(expectCurrentToken(TokenKind::Const));
    consumeAndSkipBlanks();

    TRY(expectCurrentToken(TokenKind::Identifier));
    bmcl::StringView name = _currentToken.value();
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

    skipBlanks();
    TRY(expectCurrentToken(TokenKind::Equality));
    consumeAndSkipBlanks();

    std::uintmax_t value;
    TRY(parseUnsignedInteger(&value));

    TRY(expectCurrentToken(TokenKind::SemiColon));
    consume();

    _ast->addConstant(new Constant(name, value, type.get()));

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
    consumeAndSkipBlanks();

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
                    consumeAndSkipBlanks();
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
                    //TODO: report error
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

        TRY(expectCurrentToken(TokenKind::Identifier));
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

    return new Function(name, fnType.get());
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

    TRY(parseList(TokenKind::LBrace, TokenKind::Eol, TokenKind::RBrace, block.get(), [this](ImplBlock* block) -> bool {
        Rc<Function> fn = parseFunction();
        if (!fn) {
            return false;
        }
        block->addFunction(fn.get());
        return true;
    }));

    bmcl::OptionPtr<NamedType> type = _ast->findTypeWithName(block->name());
    if (type.isNone()) {
        //TODO: report error
        BMCL_CRITICAL() << "No type found for impl block";
        return false;
    }

    _ast->addImplBlock(block.get());

    return true;
}

bool Parser::parseAlias()
{
    TRY(skipCommentsAndSpace());
    TRY(expectCurrentToken(TokenKind::Type));
    consumeAndSkipBlanks();

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

    _ast->addTopLevelType(type.get());

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

    return new ReferenceType(ReferenceKind::Reference, isMutable, pointee.get());
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
        //TODO: handle error
        reportUnexpectedTokenError(_currentToken.kind());
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
        return new ReferenceType(ReferenceKind::Pointer, isMutable, pointee.get());
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

    return new SliceType(_moduleInfo.get(), innerType.get());
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
    TRY(expectCurrentToken(TokenKind::Number));
    std::uintmax_t elementCount;
    TRY(parseUnsignedInteger(&elementCount));
    skipBlanks();
    TRY(expectCurrentToken(TokenKind::RBracket));
    consume();

    return new ArrayType(elementCount, innerType.get());
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

Rc<Type> Parser::parseBuiltinOrResolveType()
{
    auto it = _builtinTypes->btMap.find(_currentToken.value());
    if (it != _builtinTypes->btMap.end()) {
        consume();
        return it->second;
    }
    auto link = _ast->findTypeWithName(_currentToken.value());
    if (link.isNone()) {
        //TODO: report error
        BMCL_CRITICAL() << "unknown type " << _currentToken.value().toStdString();
        return nullptr;
    }
    consume();
    return link.unwrap();
}

Rc<Field> Parser::parseField()
{
    expectCurrentToken(TokenKind::Identifier);
    bmcl::StringView name = _currentToken.value();
    consumeAndSkipBlanks();
    expectCurrentToken(TokenKind::Colon);
    consumeAndSkipBlanks();

    Rc<Type> type = parseType();
    if (!type) {
        return nullptr;
    }
    Rc<Field> field = new Field(name, type.get());

    return field;
}

template <typename T>
bool Parser::parseRecordField(T* parent)
{
    Rc<Field> decl = parseField();
    if (!decl) {
        return false;
    }
    parent->addField(decl.get());
    return true;
}

bool Parser::parseEnumConstant(EnumType* parent)
{
    TRY(skipCommentsAndSpace());
    TRY(expectCurrentToken(TokenKind::Identifier));
    bmcl::StringView name = _currentToken.value();
    consumeAndSkipBlanks();

    if (currentTokenIs(TokenKind::Equality)) {
        consumeAndSkipBlanks();
        intmax_t value;
        TRY(parseSignedInteger(&value));
        //TODO: check value
        Rc<EnumConstant> constant = new EnumConstant(name, value, true);
        if (!parent->addConstant(constant.get())) {
            BMCL_CRITICAL() << "enum constant redefinition";
            return false;
        }
    } else {
        BMCL_CRITICAL() << "enum values MUST be user set";
        return false;
    }

    return true;
}

bool Parser::parseVariantField(VariantType* parent)
{
    TRY(expectCurrentToken(TokenKind::Identifier));
    bmcl::StringView name = _currentToken.value();
    consumeAndSkipBlanks();
     //TODO: peek next token

    if (currentTokenIs(TokenKind::Comma)) {
        Rc<ConstantVariantField> field = new ConstantVariantField(name);
        parent->addField(field.get());
    } else if (currentTokenIs(TokenKind::LBrace)) {
        Rc<StructVariantField> field = new StructVariantField(name);
        TRY(parseBraceList(field.get(), [this](StructVariantField* dest) {
            return parseRecordField(dest);
        }));
        parent->addField(field.get());
    } else if (currentTokenIs(TokenKind::LParen)) {
        Rc<TupleVariantField> field = new TupleVariantField(name);
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
        reportUnexpectedTokenError(_currentToken.kind());
        return false;
    }

    return true;
}

bool Parser::parseComponentField(Component* parent)
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
    TRY(expectCurrentToken(startToken));

    consume();
    TRY(skipCommentsAndSpace());
    TRY(expectCurrentToken(TokenKind::Identifier));
    Rc<T> type = new T(_currentToken.value(), _moduleInfo.get());
    consumeAndSkipBlanks();

    TRY(parseBraceList(type.get(), std::forward<F>(fieldParser)));

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
        reportCurrentTokenError("Component can have only one commands declaration");
        return false;
    }
    TRY(parseNamelessTag(TokenKind::Commands, TokenKind::Eol, parent, [this](Component* comp) {
        Rc<Function> fn = parseFunction(false);
        if (!fn) {
            return false;
        }
        comp->addCommand(fn.get());
        return true;
    }));
    return true;
}

bool Parser::parseComponentImpl(Component* parent)
{
    if(parent->implBlock().isSome()) {
        reportCurrentTokenError("Component can have only one impl declaration");
        return false;
    }
    Rc<ImplBlock> impl = new ImplBlock;
    TRY(parseNamelessTag(TokenKind::Impl, TokenKind::Eol, impl.get(), [this](ImplBlock* impl) {
        Rc<Function> fn = parseFunction(false);
        if (!fn) {
            return false;
        }
        impl->addFunction(fn.get());
        return true;
    }));
    parent->setImplBlock(impl.get());
    return true;
}

bool Parser::parseParameters(Component* parent)
{
    if(parent->hasParams()) {
        reportCurrentTokenError("Component can have only one parameters declaration");
        return false;
    }
    TRY(parseNamelessTag(TokenKind::Parameters, TokenKind::Comma, parent, [this](Component* comp) {
        Rc<Field> field = parseField();
        if (!field) {
            return false;
        }
        comp->addParam(field.get());
        return true;
    }));
    return true;
}

bool Parser::parseStatuses(Component* parent)
{
    if(parent->hasStatuses()) {
        reportCurrentTokenError("Component can have only one statuses declaration");
        return false;
    }
    TRY(parseNamelessTag(TokenKind::Statuses, TokenKind::Comma, parent, [this](Component* comp) -> bool {
        TRY(expectCurrentToken(TokenKind::LBracket));
        consumeAndSkipBlanks();

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
            BMCL_DEBUG() << "invalid bool param";
            return false;
        }

        consumeAndSkipBlanks();
        TRY(expectCurrentToken(TokenKind::RBracket));

        StatusMsg* msg = new StatusMsg(num, prio, isEnabled);
        bool isOk = comp->addStatus(num, msg);
        if (!isOk) {
            BMCL_CRITICAL() << "redefinition of status param";
            return false;
        }
        consumeAndSkipBlanks();
        TRY(expectCurrentToken(TokenKind::Colon));
        consumeAndSkipBlanks();
        auto parseOneRegexp = [this, comp](StatusMsg* msg) -> bool {
            if (!currentTokenIs(TokenKind::Identifier)) {
                //TODO: report error
                return false;
            }
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
    return true;
}

//TODO: make component numbers explicit

bool Parser::parseComponent()
{
    TRY(expectCurrentToken(TokenKind::Component));
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
            //TODO: report error
            return false;
        }
    }

finish:
    //TODO: only one component allowed, add check
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




