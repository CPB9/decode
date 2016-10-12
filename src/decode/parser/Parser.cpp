#include "decode/parser/Parser.h"
#include "decode/parser/Decl.h"
#include "decode/parser/Ast.h"
#include "decode/parser/Token.h"
#include "decode/parser/Decl.h"
#include "decode/parser/Type.h"
#include "decode/parser/Lexer.h"
#include "decode/parser/FileInfo.h"
#include "decode/parser/ModuleInfo.h"

#include <bmcl/FileUtils.h>
#include <bmcl/Logging.h>
#include <bmcl/Result.h>

#define TRY(expr) \
    do { \
        if (!expr) { \
            return false; \
        } \
    } while(0);

namespace decode {
namespace parser {

Parser::Parser()
{
}

Parser::~Parser()
{
}

void Parser::reportUnexpectedTokenError(TokenKind expected)
{
}

ParseResult<Rc<Ast>> Parser::parseFile(const char* fname)
{
    if (parseOneFile(fname)) {
        return std::move(_ast);
    }
    return ParseResult<Rc<Ast>>();
}

void Parser::consume()
{
    _lexer->consumeNextToken(&_currentToken);
}

bool Parser::skipCommentsAndSpace()
{
    while (true) {
        switch (_currentToken.kind()) {
        case TokenKind::Blank:
            break;
        case TokenKind::Eol:
            break;
        case TokenKind::RawComment:
            break;
        case TokenKind::DocComment:
            break;
        case TokenKind::Eof:
            return true;
        case TokenKind::Invalid:
            //TODO: report error
            return false;
        default:
            return true;
        }
        consume();
    }
    return true;
}

void Parser::consumeAndSkipBlanks()
{
    consume();
    while (_currentToken.kind() == TokenKind::Blank) {
        consume();
    }
}

bool Parser::consumeAndExpectCurrentToken(TokenKind expected)
{
    _lexer->consumeNextToken(&_currentToken);
    return expectCurrentToken(expected);
}

bool Parser::expectCurrentToken(TokenKind expected)
{
    if (_currentToken.kind() != expected) {
        reportUnexpectedTokenError(expected);
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

bool Parser::consumeAndSetNamedDeclName(NamedDecl* decl)
{
    TRY(consumeAndExpectCurrentToken(TokenKind::Identifier));
    decl->_name = _currentToken.value();
    return true;
}

bool Parser::parseModuleDecl()
{
    TRY(skipCommentsAndSpace());
    _moduleInfo = new ModuleInfo;
    _moduleInfo->_fileInfo = _fileInfo;
    _ast->_moduleInfo = _moduleInfo;
    Rc<ModuleDecl> modDecl = beginDecl<ModuleDecl>();
    TRY(expectCurrentToken(TokenKind::Module));
    TRY(consumeAndExpectCurrentToken(TokenKind::Blank));
    TRY(consumeAndSetNamedDeclName(modDecl.get()));
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
        Rc<ImportDecl> importDecl = beginDecl<ImportDecl>();
        TRY(consumeAndExpectCurrentToken(TokenKind::Blank));
        TRY(consumeAndExpectCurrentToken(TokenKind::Identifier));
        importDecl->_importPath = _currentToken.value();
        TRY(consumeAndExpectCurrentToken(TokenKind::DoubleColon));
        consume();

        auto createImportedTypeFromCurrentToken = [this, importDecl]() {
            Rc<ImportedType> type = new ImportedType;
            type->_importPath = importDecl->_importPath;
            type->_name = _currentToken.value();
            importDecl->_types.push_back(std::move(type));
        };

        if (_currentToken.kind() == TokenKind::Identifier) {
            createImportedTypeFromCurrentToken();
        } else if (_currentToken.kind() == TokenKind::LBrace) {
            consume();
            while (true) {
                TRY(expectCurrentToken(TokenKind::Identifier));
                createImportedTypeFromCurrentToken();
                consumeAndSkipBlanks();
                if (_currentToken.kind() == TokenKind::Comma) {
                    consumeAndSkipBlanks();
                    continue;
                } else if (_currentToken.kind() == TokenKind::RBrace) {
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

        consumeAndEndDecl(importDecl);
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
            case TokenKind::Eol:
                return true;
            default:
                BMCL_CRITICAL() << "Unexpected top level decl token";
                //TODO: report error
                return false;
        }
    }
    return true;
}

bool Parser::parseStruct()
{
    Rc<StructDecl> structDecl = beginDecl<StructDecl>();
    TRY(expectCurrentToken(TokenKind::Struct));

    consume();
    TRY(skipCommentsAndSpace());
    TRY(expectCurrentToken(TokenKind::Identifier));
    structDecl->_name = _currentToken.value();

    consume();
    TRY(skipCommentsAndSpace());
    TRY(expectCurrentToken(TokenKind::LBrace));

    consume();
    TRY(skipCommentsAndSpace());

    return true;
}

bool Parser::parseOneFile(const char* fname)
{
    bmcl::Result<std::string, int> rv = bmcl::readFileIntoString(fname);
    if (rv.isErr()) {
        return false;
    }

    _fileInfo = new FileInfo(fname, rv.unwrap());
    _lexer = new Lexer(bmcl::StringView(rv.unwrap()));
    _ast = new Ast;

    _lexer->consumeNextToken(&_currentToken);
    TRY(parseModuleDecl());
    TRY(parseImports());
    TRY(parseTopLevelDecls());

    _lexer = nullptr;
    _fileInfo = nullptr;
    _moduleInfo = nullptr;
    return true;
}
}
}
