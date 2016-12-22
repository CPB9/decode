#pragma once

#include "decode/Config.h"
#include "decode/parser/AstVisitor.h"
#include "decode/generator/SrcBuilder.h"

namespace decode {

template <typename B>
class NameVisitor : public ConstAstVisitor<B> {
public:
    NameVisitor(SrcBuilder* dest);

    B& base();

    bool visitFunctionType(const FunctionType* type);
    bool visitEnumType(const EnumType* type);
    bool visitStructType(const StructType* type);
    bool visitVariantType(const VariantType* type);
    bool visitImportedType(const ImportedType* type);

protected:

    bool appendTypeName(const Type* type);

    SrcBuilder* _output;
};

template <typename B>
inline NameVisitor<B>::NameVisitor(SrcBuilder* dest)
    : _output(dest)
{
}

template <typename B>
inline B& NameVisitor<B>::base()
{
    return *static_cast<B*>(this);
}

template <typename B>
inline bool NameVisitor<B>::visitFunctionType(const FunctionType* type)
{
    return base().appendTypeName(type);
}

template <typename B>
inline bool NameVisitor<B>::visitEnumType(const EnumType* type)
{
    return base().appendTypeName(type);
}

template <typename B>
inline bool NameVisitor<B>::visitStructType(const StructType* type)
{
    return base().appendTypeName(type);
}

template <typename B>
inline bool NameVisitor<B>::visitVariantType(const VariantType* type)
{
    return base().appendTypeName(type);
}

template <typename B>
inline bool NameVisitor<B>::visitImportedType(const ImportedType* type)
{
    return base().appendTypeName(type);
}
}
