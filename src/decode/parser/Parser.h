/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "decode/Config.h"
#include "decode/core/Rc.h"
#include "decode/core/Hash.h"
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
class ModuleDecl;
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
class FieldVec;
class StructType;
class FunctionType;
class TypeDecl;
class Ast;
class Constant;
class CfgOption;
class Function;

enum class BuiltinTypeKind;

using ParseResult = bmcl::Result<Rc<Ast>, void>;

struct AllBuiltinTypes : public RefCountable {
    Rc<BuiltinType> usizeType;
    Rc<BuiltinType> isizeType;
    Rc<BuiltinType> varuintType;
    Rc<BuiltinType> varintType;
    Rc<BuiltinType> u8Type;
    Rc<BuiltinType> i8Type;
    Rc<BuiltinType> u16Type;
    Rc<BuiltinType> i16Type;
    Rc<BuiltinType> u32Type;
    Rc<BuiltinType> i32Type;
    Rc<BuiltinType> u64Type;
    Rc<BuiltinType> i64Type;
    Rc<BuiltinType> boolType;
    Rc<BuiltinType> voidType;
    std::unordered_map<bmcl::StringView, Rc<BuiltinType>> btMap;
};

class DECODE_EXPORT Parser {
public:
    Parser(Diagnostics* diag);
    ~Parser();

    ParseResult parseFile(const char* fileName);
    ParseResult parseFile(FileInfo* finfo);

    Location currentLoc() const; //FIXME: temp

private:
    bool parseOneFile(FileInfo* finfo);
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
    bool parseAttribute();
    Rc<CfgOption> parseCfgOption();

    template <typename T, typename F>
    bool parseList(TokenKind openToken, TokenKind sep, TokenKind closeToken, T&& parent, F&& fieldParser);

    template <typename T, typename F>
    bool parseBraceList(T&& parent, F&& fieldParser);

    template <typename T, bool genericAllowed, typename F>
    bool parseTag(TokenKind startToken, F&& fieldParser);

    template <typename T, bool genericAllowed, typename F>
    bool parseTag2(TokenKind startToken, F&& fieldParser);

    template<typename T, typename F>
    bool parseNamelessTag(TokenKind startToken, TokenKind sep, T* dest, F&& parser);

    Rc<Field> parseField();

    template <typename T>
    bool parseRecordField(T* parent);
    bool parseEnumConstant(EnumType* parent);
    bool parseVariantField(VariantType* parent);
    bool parseComponentField(Component* parent);

    Rc<Function> parseFunction(bool selfAllowed = true);

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

    bool parseParameters(Component* parent);
    bool parseCommands(Component* parent);
    bool parseStatuses(Component* parent);
    bool parseComponentImpl(Component* parent);

    template <typename T>
    Rc<T> beginDecl();
    template <typename T>
    void consumeAndEndDecl(const Rc<T>& decl);
    template <typename T>
    void endDecl(const Rc<T>& decl);

    template <typename T>
    Rc<T> beginType();
    template <typename T>
    Rc<T> beginNamedType(bmcl::StringView name);
    template <typename T>
    void consumeAndEndType(const Rc<T>& type);
    template <typename T>
    void endType(const Rc<T>& type);


    bool currentTokenIs(TokenKind kind);

    void finishSplittingLines();

    Rc<Report> reportCurrentTokenError(const char* msg);

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
