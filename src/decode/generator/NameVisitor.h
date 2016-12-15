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

    bool visitFunction(const Function* type);
    bool visitEnum(const Enum* type);
    bool visitStruct(const StructType* type);
    bool visitVariant(const Variant* type);
    bool visitUnresolved(const UnresolvedType* type);

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
inline bool NameVisitor<B>::visitFunction(const Function* type)
{
    return base().appendTypeName(type);
}

template <typename B>
inline bool NameVisitor<B>::visitEnum(const Enum* type)
{
    return base().appendTypeName(type);
}

template <typename B>
inline bool NameVisitor<B>::visitStruct(const StructType* type)
{
    return base().appendTypeName(type);
}

template <typename B>
inline bool NameVisitor<B>::visitVariant(const Variant* type)
{
    return base().appendTypeName(type);
}

template <typename B>
inline bool NameVisitor<B>::visitUnresolved(const UnresolvedType* type)
{
    return base().appendTypeName(type);
}
}
