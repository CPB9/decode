#pragma once

#include "decode/Config.h"
#include "decode/core/Target.h"
#include "decode/parser/AstVisitor.h"
#include "decode/generator/SrcBuilder.h"
#include "decode/generator/TypeReprGen.h"

#include <stack>

namespace decode {

template <typename B>
class InlineTypeInspector : public ConstAstVisitor<B> {
public:
    InlineTypeInspector(const Target* target, SrcBuilder* output);

    void inspect(const Type* type, const InlineSerContext& ctx, bmcl::StringView argName);

    B& base();

    bool visitArrayType(const ArrayType* type);
    bool visitFunctionType(const FunctionType* type);
    bool visitReferenceType(const ReferenceType* type);
    bool visitEnumType(const EnumType* type);
    bool visitStructType(const StructType* type);
    bool visitVariantType(const VariantType* type);
    bool visitSliceType(const SliceType* type);
    bool visitBuiltinType(const BuiltinType* type);

    void inspectPointer(const Type* type);
    void inspectNonInlineType(const Type* type);

    void genSizedSer(std::size_t pointerSize, bmcl::StringView suffix);
    void genVarSer(bmcl::StringView suffix);

protected:

    const InlineSerContext& context() const;

    void appendArgumentName();

    bool appendTypeName(const Type* type);

    void popArgName(std::size_t n);

    void appendTypeRepr(const Type* type);

    std::stack<InlineSerContext> _ctxStack;
    const Target* _target;
    SrcBuilder* _output;
    std::string _argName;
    TypeReprGen _reprGen;
};

template <typename B>
inline void InlineTypeInspector<B>::appendArgumentName()
{
    _output->append(_argName);
}

template <typename B>
inline void InlineTypeInspector<B>::popArgName(std::size_t n)
{
    _argName.erase(_argName.size() - n, n);
}

template <typename B>
inline const InlineSerContext& InlineTypeInspector<B>::context() const
{
    return _ctxStack.top();
}

template <typename B>
inline void InlineTypeInspector<B>::appendTypeRepr(const Type* type)
{
    _reprGen.genTypeRepr(type);
}

template <typename B>
inline B& InlineTypeInspector<B>::base()
{
    return *static_cast<B*>(this);
}

template <typename B>
InlineTypeInspector<B>::InlineTypeInspector(const Target* target, SrcBuilder* output)
    : _target(target)
    , _output(output)
    , _reprGen(output)
{
}

template <typename B>
void InlineTypeInspector<B>::inspect(const Type* type, const InlineSerContext& ctx, bmcl::StringView argName)
{
    assert(_ctxStack.size() == 0);
    _ctxStack.push(ctx);
    _argName.assign(argName.begin(), argName.end());
    base().traverseType(type);
    _ctxStack.pop();
}

template <typename B>
inline bool InlineTypeInspector<B>::visitFunctionType(const FunctionType* type)
{
    base().inspectPointer(type);
    return false;
}

template <typename B>
inline bool InlineTypeInspector<B>::visitReferenceType(const ReferenceType* type)
{
    base().inspectPointer(type);
    return false;
}

template <typename B>
bool InlineTypeInspector<B>::visitArrayType(const ArrayType* type)
{
    _output->appendLoopHeader(context(), type->elementCount());

    _argName.push_back('[');
    _argName.push_back(context().currentLoopVar());
    _argName.push_back(']');
    _ctxStack.push(context().indent().incLoopVar());
    base().traverseType(type->elementType().get());
    popArgName(3);
    _ctxStack.pop();
    _output->appendIndent(context());
    _output->append("}\n");
    return false;
}

template <typename B>
inline bool InlineTypeInspector<B>::visitStructType(const StructType* type)
{
    base().inspectNonInlineType(type);
    return false;
}

template <typename B>
inline bool InlineTypeInspector<B>::visitVariantType(const VariantType* type)
{
    base().inspectNonInlineType(type);
    return false;
}

template <typename B>
inline bool InlineTypeInspector<B>::visitEnumType(const EnumType* type)
{
    base().inspectNonInlineType(type);
    return false;
}

template <typename B>
inline bool InlineTypeInspector<B>::visitSliceType(const SliceType* type)
{
    base().inspectNonInlineType(type);
    return false;
}

template <typename B>
bool InlineTypeInspector<B>::visitBuiltinType(const BuiltinType* type)
{
    switch (type->builtinTypeKind()) {
    case BuiltinTypeKind::USize:
        base().genSizedSer(_target->pointerSize(), "USizeLe");
        break;
    case BuiltinTypeKind::ISize:
        base().genSizedSer(_target->pointerSize(), "USizeLe");
        break;
    case BuiltinTypeKind::U8:
        base().genSizedSer(1, "U8");
        break;
    case BuiltinTypeKind::I8:
        base().genSizedSer(1, "U8");
        break;
    case BuiltinTypeKind::U16:
        base().genSizedSer(2, "U16Le");
        break;
    case BuiltinTypeKind::I16:
        base().genSizedSer(2, "U16Le");
        break;
    case BuiltinTypeKind::U32:
        base().genSizedSer(4, "U32Le");
        break;
    case BuiltinTypeKind::I32:
        base().genSizedSer(4, "U32Le");
        break;
    case BuiltinTypeKind::U64:
        base().genSizedSer(8, "U64Le");
        break;
    case BuiltinTypeKind::I64:
        base().genSizedSer(8, "U64Le");
        break;
    case BuiltinTypeKind::Bool:
        base().genSizedSer(1, "U8");
        break;
    case BuiltinTypeKind::Varuint:
        base().genVarSer("Varuint");
        break;
    case BuiltinTypeKind::Varint:
        base().genVarSer("Varint");
        break;
    case BuiltinTypeKind::Void:
        //TODO: disallow
        assert(false);
        break;
    case BuiltinTypeKind::Unknown:
        assert(false);
        break;
    default:
        assert(false);
    }
    return false;
}

}

