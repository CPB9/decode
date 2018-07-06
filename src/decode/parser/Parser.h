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
#include "decode/parser/Token.h"
#include "decode/core/Iterator.h"
#include "decode/core/HashMap.h"
#include "decode/parser/Containers.h"

#include <bmcl/Fwd.h>
#include <bmcl/StringView.h>
#include <bmcl/StringViewHash.h>

#include <vector>
#include <cstdint>

namespace decode {

class Ast;
class BuiltinType;
class CfgOption;
class Component;
class Constant;
class Decl;
class Diagnostics;
class DocBlock;
class EnumConstant;
class EnumType;
class Field;
class FieldVec;
class FileInfo;
class Function;
class FunctionType;
class GenericParameterType;
class ImportedType;
class Lexer;
class ModuleDecl;
class ModuleInfo;
class NamedDecl;
class RangeAttr;
class Report;
class StructDecl;
class StructType;
class Type;
class TypeDecl;
class VariantType;
class AllBuiltinTypes;
class CmdCallAttr;
class Parameter;
class VarRegexp;

enum class BuiltinTypeKind;
enum class ReferenceKind;

using ParseResult = bmcl::Result<Rc<Ast>, void>;

class DECODE_EXPORT Parser {
public:
    Parser(Diagnostics* diag);
    Parser(Parser&& other) = delete; // msvc 2015 hack
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
    bool consumeAndExpectCurrentToken(TokenKind expected, const char* msg);
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
    Rc<RangeAttr> parseRangeAttr();
    Rc<CmdCallAttr> parseCmdCallAttr();

    bool parseParameter(Component* comp);
    bool parseParameterPath(Component* comp, Parameter* param);

    template <typename T, typename F, typename... A>
    bool parseList(TokenKind openToken, TokenKind sep, TokenKind closeToken, T&& parent, F&& fieldParser, A&&... args);

    template <typename T, typename F, typename... A>
    bool parseList2(TokenKind sep, TokenKind closeToken, T&& parent, F&& fieldParser, A&&... args);

    template <typename T, typename F, typename... A>
    bool parseBraceList(T&& parent, F&& fieldParser, A&&... args);

    template <typename T, bool genericAllowed, typename F, typename... A>
    bool parseTag(TokenKind startToken, F&& fieldParser, A&&... args);

    template<typename T, typename F>
    bool parseNamelessTag(TokenKind startToken, TokenKind sep, T* dest, F&& parser);

    Rc<Field> parseField();

    template <typename T>
    bool parseRecordField(T* parent);
    bool parseEnumConstant(EnumType* parent, std::intmax_t* current);
    bool parseVariantField(VariantType* parent);
    bool parseComponentField(Component* parent);

    template <typename T>
    Rc<T> parseFunction(bool selfAllowed = true);

    Rc<VarRegexp> parseVarRegexp();

    Rc<Type> parseType();
    Rc<Type> parseFunctionPointer();
    Rc<Type> parseReferenceType();
    Rc<Type> parsePointerType();
    Rc<Type> parseNonReferenceType();
    Rc<Type> parseDynArrayType();
    Rc<Type> parseArrayType();
    Rc<Type> parseBuiltinOrResolveType();
    bool parseUnsignedInteger(std::uintmax_t* dest);
    bool parseSignedInteger(std::intmax_t* dest);

    bool parseVariables(Component* parent);
    bool parseCommands(Component* parent);
    bool parseStatuses(Component* parent);
    bool parseEvents(Component* parent);
    bool parseComponentImpl(Component* parent);
    bool parseParameters(Component* parent);
    bool parseSavedVars(Component* parent);

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

    bool parseBoolean(bool* value);

    bool currentTokenIs(TokenKind kind);

    void finishSplittingLines();

    Rc<DocBlock> createDocsFromComments();
    void clearUnusedDocCommentsAndAttributes();
    void clearGenericParameters();

    Rc<Report> reportCurrentTokenError(const char* msg);
    Rc<Report> reportCurrentTokenError(const std::string& str);
    Rc<Report> reportTokenError(Token* tok, const char* msg);

    Rc<Diagnostics> _diag;

    Token _currentToken;
    Rc<Lexer> _lexer;
    Rc<Ast> _ast;
    Rc<FileInfo> _fileInfo;
    Rc<ModuleInfo> _moduleInfo;

    Rc<AllBuiltinTypes> _builtinTypes;

    const char* _lastLineStart;
    std::size_t _currentTmMsgNum;
    std::size_t _currentCmdRegexpNum;
    std::vector<bmcl::StringView> _docComments;
    RcVec<GenericParameterType> _currentGenericParameters;
    Rc<RangeAttr> _lastRangeAttr;
    Rc<CmdCallAttr> _lastCmdCallAttr;
    HashMap<bmcl::StringView, Rc<BuiltinType>> _btMap;
};
}
