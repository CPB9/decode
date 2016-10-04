#pragma once

#include <pegtl/input.hh>
#include <pegtl/parse.hh>
#include <pegtl/rules.hh>
#include <pegtl/ascii.hh>

namespace decode {
namespace grammar {

struct Space
        : pegtl::space {};

struct OptSpace
        : pegtl::opt<Space> {};

template<typename T>
struct Padded
        : pegtl::pad<T, Space> {};

struct Comma
        : pegtl::one<','> {};

struct Colon
        : pegtl::one<':'> {};

struct LBrace
        : pegtl::one<'{'> {};

struct RBrace
        : pegtl::one<'}'> {};

struct Identifier
        : pegtl::identifier {};

struct FieldIdentifier
        : Identifier {};

struct TypeName
        : Identifier {};

struct TypeDecl
        : TypeName {}; //TODO: pointers

struct StructFieldDecl
        : pegtl::seq<FieldIdentifier, Padded<Colon>, TypeDecl> {};

struct StructFieldDeclList
        : pegtl::list_tail<StructFieldDecl, Comma, pegtl::space> {};

struct StructLiteral
        : pegtl_string_t("struct") {};

struct StructTypeName
        : Identifier {};

struct StructDecl
        : pegtl::seq<StructLiteral, Space, StructTypeName,
                     Padded<LBrace>, StructFieldDeclList, Space, RBrace> {};

//struct Grammar
//        : pegtl::must<StructDecl, pegtl::eof> {};

}
}
