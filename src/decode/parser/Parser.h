#pragma once

#include "decode/Config.h"
#include "decode/core/Rc.h"
#include "decode/parser/Token.h"

#include <bmcl/StringView.h>
#include <bmcl/ResultFwd.h>

#include <vector>
#include <cstdint>
#include <unordered_map>

namespace decode {

enum class ReferenceKind;

class Lexer;
class Decl;
class Ast;
class Module;
class NamedDecl;
class FileInfo;
class ModuleInfo;
class EnumConstant;
class EnumType;
class Component;
class StructDecl;
class VariantType;
class ImportedType;
class Type;
class Report;
class Diagnostics;
class BuiltinType;
class Field;
class FieldList;
class StructType;
class FunctionType;
class TypeDecl;
class Ast;
class Constant;

enum class BuiltinTypeKind;

typedef bmcl::Result<Rc<Ast>, void> ParseResult;

struct AllBuiltinTypes;

class DECODE_EXPORT Parser {
public:
    Parser(const Rc<Diagnostics>& diag);
    ~Parser();

    ParseResult parseFile(const char* fileName);
    ParseResult parseFile(const Rc<FileInfo>& finfo);

    Location currentLoc() const; //FIXME: temp

private:
    bool parseOneFile(const Rc<FileInfo>& finfo);
    void cleanup();

    bool expectCurrentToken(TokenKind expected);
    bool expectCurrentToken(TokenKind expected, const char* msg);
    bool consumeAndExpectCurrentToken(TokenKind expected);
    void reportUnexpectedTokenError(TokenKind expected);

    void addLine();

    void consume();
    bool skipCommentsAndSpace();
    void consumeAndSkipBlanks();
    void skipBlanks();
    bool consumeAndSetNamedDeclName(NamedDecl* decl);

    bool parseModuleDecl();
    bool parseImports();
    bool parseTopLevelDecls();
    bool parseStruct();
    bool parseEnum();
    bool parseVariant();
    bool parseComponent();
    bool parseImplBlock();
    bool parseAlias();
    bool parseConstant();

    template <typename T, typename F>
    bool parseList(TokenKind openToken, TokenKind sep, TokenKind closeToken, T&& parent, F&& fieldParser);

    template <typename T, typename F>
    bool parseBraceList(T&& parent, F&& fieldParser);

    template <typename T, bool genericAllowed, typename F>
    bool parseTag(TokenKind startToken, F&& fieldParser);

    template<typename T, typename F>
    Rc<T> parseNamelessTag(TokenKind startToken, TokenKind sep, F&& parser);

    Rc<Field> parseField();
    bool parseRecordField(const Rc<FieldList>& parent);
    bool parseEnumConstant(const Rc<EnumType>& parent);
    bool parseVariantField(const Rc<VariantType>& parent);
    bool parseComponentField(const Rc<Component>& parent);

    Rc<FunctionType> parseFunction(bool selfAllowed = true);

    Rc<Type> parseType();
    Rc<Type> parseFunctionPointer();
    Rc<Type> parseReferenceType();
    Rc<Type> parsePointerType();
    Rc<Type> parseNonReferenceType();
    Rc<Type> parseSliceType();
    Rc<Type> parseArrayType();
    Rc<Type> parseBuiltinOrResolveType();
    bool parseUnsignedInteger(std::uintmax_t* dest);
    bool parseSignedInteger(std::intmax_t* dest);

    bool parseParameters(const Rc<Component>& parent);
    bool parseCommands(const Rc<Component>& parent);
    bool parseStatuses(const Rc<Component>& parent);

    template <typename T>
    Rc<T> beginDecl();
    template <typename T>
    void consumeAndEndDecl(const Rc<T>& decl);
    template <typename T>
    void endDecl(const Rc<T>& decl);

    template <typename T>
    Rc<T> beginType();
    template <typename T>
    Rc<T> beginNamedType();
    template <typename T>
    void consumeAndEndType(const Rc<T>& type);
    template <typename T>
    void endType(const Rc<T>& type);


    bool currentTokenIs(TokenKind kind);

    void finishSplittingLines();

    Rc<Report> reportCurrentTokenError(const char* msg);

    Rc<Type> findDeclaredType(bmcl::StringView name) const;

    Rc<Diagnostics> _diag;

    Token _currentToken;
    Rc<Lexer> _lexer;
    Rc<Ast> _ast;
    Rc<FileInfo> _fileInfo;
    Rc<ModuleInfo> _moduleInfo;

    std::vector<Rc<TypeDecl>> _typeDeclStack;
    Rc<AllBuiltinTypes> _builtinTypes;

    const char* _lastLineStart;
};
}
