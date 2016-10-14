#pragma once

#include "decode/Config.h"
#include "decode/Rc.h"
#include "decode/parser/Token.h"

#include <bmcl/StringView.h>
#include <bmcl/ResultFwd.h>

#include <vector>
#include <cstdint>

namespace decode {
namespace parser {

enum class ReferenceType;

class Lexer;
class Decl;
class Ast;
class ModuleDecl;
class NamedDecl;
class FileInfo;
class ModuleInfo;
class ImportedType;
class Type;

template <typename T>
using ParseResult = bmcl::Result<T, void>;

class DECODE_EXPORT Parser {
public:
    Parser();
    ~Parser();

    ParseResult<Rc<Ast>> parseFile(const char* fileName);
    bool parseOneFile(const char* fileName);

private:

    bool expectCurrentToken(TokenKind expected);
    bool consumeAndExpectCurrentToken(TokenKind expected);
    void reportUnexpectedTokenError(TokenKind expected);

    void consume();
    bool skipCommentsAndSpace();
    void consumeAndSkipBlanks();
    void skipBlanks();
    bool consumeAndSetNamedDeclName(NamedDecl* decl);

    bool parseModuleDecl();
    bool parseImports();
    bool parseTopLevelDecls();
    bool parseStruct();
    Rc<Type> parseType();
    Rc<Type> parseArrayType();
    Rc<Type> parseSliceType();
    Rc<Type> parseBuiltinOrResolveType();
    Rc<Type> parseSliceOrBuiltinOrResolveType(ReferenceType refType);
    bool parseArraySize(std::uintmax_t* dest);

    template <typename T>
    Rc<T> beginDecl();

    template <typename T>
    void consumeAndEndDecl(const Rc<T>& decl);

    Token _currentToken;
    Rc<Lexer> _lexer;
    Rc<Ast> _ast;
    Rc<FileInfo> _fileInfo;
    Rc<ModuleInfo> _moduleInfo;
};
}
}
