#include "decode/generator/InlineTypeInspector.h"

#include "decode/ast/Type.h"
#include "decode/generator/SrcBuilder.h"
#include "decode/generator/TypeReprGen.h"
#include "decode/generator/TypeNameGen.h"

namespace decode {

InlineTypeInspector::InlineTypeInspector(SrcBuilder* output)
    : _output(output)
{
}

bool InlineTypeInspector::isSizeCheckEnabled() const
{
    return _checkSizes;
}

void InlineTypeInspector::appendArgumentName()
{
    _output->append(_argName);
}

void InlineTypeInspector::popArgName(std::size_t n)
{
    _argName.erase(_argName.size() - n, n);
}

const InlineSerContext& InlineTypeInspector::context() const
{
    return _ctxStack.top();
}

template <bool isOnboard, bool isSerializer>
void InlineTypeInspector::inspectType(const Type* type)
{
    switch (type->typeKind()) {
    case TypeKind::Builtin:
        if (isOnboard) {
            inspectOnboardBuiltin<isSerializer>(type->asBuiltin());
        } else {
            inspectGcBuiltin<isSerializer>(type->asBuiltin());
        }
        break;
    case TypeKind::Reference:
        inspectPointer<isOnboard, isSerializer>(type);
        break;
    case TypeKind::Array:
        inspectArray<isOnboard, isSerializer>(type->asArray());
        break;
    case TypeKind::DynArray:
        if (isOnboard) {
            inspectOnboardNonInlineType<isSerializer>(type);
        } else {
            inspectGcDynArray<isSerializer>(type->asDynArray());
        }
        break;
    case TypeKind::Function:
        inspectPointer<isOnboard, isSerializer>(type);
        break;
    case TypeKind::Enum:
        inspectNonInlineType<isOnboard, isSerializer>(type->asEnum());
        break;
    case TypeKind::Struct:
        inspectNonInlineType<isOnboard, isSerializer>(type->asStruct());
        break;
    case TypeKind::Variant:
        inspectNonInlineType<isOnboard, isSerializer>(type->asVariant());
        break;
    case TypeKind::Imported:
        inspectType<isOnboard, isSerializer>(type->asImported()->link());
        break;
    case TypeKind::Alias:
        if (isOnboard) {
            inspectType<isOnboard, isSerializer>(type->asAlias()->alias());
        } else {
            inspectNonInlineType<isOnboard, isSerializer>(type->asAlias());
        }
        break;
    case TypeKind::Generic:
        inspectNonInlineType<isOnboard, isSerializer>(type->asGeneric());
        break;
    case TypeKind::GenericInstantiation:
        if (isOnboard) {
            inspectOnboardNonInlineType<isSerializer>(type);
        } else {
            inspectNonInlineType<isOnboard, isSerializer>(type);
        }
        break;
    case TypeKind::GenericParameter:
        break;
    }
}

void InlineTypeInspector::genOnboardSerializer(const Type* type, const InlineSerContext& ctx, bmcl::StringView argName, bool checkSizes)
{
    inspect<true, true>(type, ctx, argName, checkSizes);
}

void InlineTypeInspector::genOnboardDeserializer(const Type* type, const InlineSerContext& ctx, bmcl::StringView argName, bool checkSizes)
{
    inspect<true, false>(type, ctx, argName, checkSizes);
}

template <bool isOnboard, bool isSerializer>
void InlineTypeInspector::inspect(const Type* type, const InlineSerContext& ctx, bmcl::StringView argName, bool checkSizes)
{
    assert(_ctxStack.size() == 0);
    _ctxStack.push(ctx);
    _argName.assign(argName.begin(), argName.end());
    _checkSizes = checkSizes;
    inspectType<isOnboard, isSerializer>(type);
    _ctxStack.pop();
}

template <bool isOnboard, bool isSerializer>
void InlineTypeInspector::appendSizeCheck(const InlineSerContext& ctx, bmcl::StringView name, SrcBuilder* dest)
{
    if (isOnboard) {
        if (isSerializer) {
            dest->appendWritableSizeCheck(ctx, name);
        } else {
            dest->appendReadableSizeCheck(ctx, name);
        }
    } else {
        //TODO: refact, report error
        if (isSerializer) {
//             dest->appendIndent(ctx);
//             dest->append("if (dest->sizeLeft() < ");
//             dest->append(name);
//             dest->append(") {\n");
//             dest->appendIndent(ctx);
//             dest->append("    return false;\n");
//             dest->appendIndent(ctx);
//             dest->append("}\n");
        } else {
            dest->appendIndent(ctx);
            dest->append("if (src->sizeLeft() < ");
            dest->append(name);
            dest->append(") {\n");
            dest->appendIndent(ctx);
            dest->append("    return false;\n");
            dest->appendIndent(ctx);
            dest->append("}\n");
        }
    }
}

template <bool isSerializer>
void InlineTypeInspector::inspectGcDynArray(const DynArrayType* type)
{
    _output->appendIndent(context());
    _output->append("{\n");
    _ctxStack.push(context().indent());
    _output->appendIndent(context());
    if (isSerializer) {
        _output->append("uint64_t _size = ");
        appendArgumentName();
        _output->append(".size();\n");
        _output->appendIndent(context());
        _output->append("dest->writeVarUint(_size);\n");
    } else {
        _output->append("uint64_t _size;\n");
        _output->appendIndent(context());
        _output->append("if (!src->readVarUint(&_size)) {\n");
        _output->appendIndent(context());
        _output->append("    return false;\n");
        _output->appendIndent(context());
        _output->append("}\n");
        _output->appendIndent(context());
        appendArgumentName();
        _output->append(".resize(_size);\n");
    }
    _output->appendLoopHeader(context(), "_size");
    _argName.push_back('[');
    _argName.push_back(context().currentLoopVar());
    _argName.push_back(']');
    _ctxStack.push(context().indent().incLoopVar());
    inspectType<false, isSerializer>(type->elementType());
    _ctxStack.pop();
    _output->appendIndent(context());
    _output->append("}\n");
    _ctxStack.pop();
    _output->appendIndent(context());
    _output->append("}\n");
}

template <bool isOnboard, bool isSerializer>
void InlineTypeInspector::inspectArray(const ArrayType* type)
{
    _argName.push_back('[');
    _argName.push_back(context().currentLoopVar());
    _argName.push_back(']');
    bool oldCheckSizes = _checkSizes;
    if (_checkSizes) {
        _checkSizes = false;
        auto size = type->elementType()->fixedSize();
        if (size.isSome()) {
            appendSizeCheck<isOnboard, isSerializer>(context(), std::to_string(size.unwrap() * type->elementCount()), _output);
        }
    }
    _output->appendLoopHeader(context(), type->elementCount());
    _ctxStack.push(context().indent().incLoopVar());
    inspectType<isOnboard, isSerializer>(type->elementType());
    _checkSizes = oldCheckSizes;
    popArgName(3);
    _ctxStack.pop();
    _output->appendIndent(context());
    _output->append("}\n");
}

template <bool isSerializer>
void InlineTypeInspector::inspectGcBuiltin(const BuiltinType* type)
{
    switch (type->builtinTypeKind()) {
    case BuiltinTypeKind::USize:
        //HACK
        genGcSizedSer<isSerializer>("8", "Uint64Le");
        break;
    case BuiltinTypeKind::ISize:
        //HACK
        genGcSizedSer<isSerializer>("8", "Int64Le");
        break;
    case BuiltinTypeKind::U8:
        genGcSizedSer<isSerializer>("1", "Uint8");
        break;
    case BuiltinTypeKind::I8:
        genGcSizedSer<isSerializer>("1", "Int8");
        break;
    case BuiltinTypeKind::U16:
        genGcSizedSer<isSerializer>("2", "Uint16Le");
        break;
    case BuiltinTypeKind::I16:
        genGcSizedSer<isSerializer>("2", "Int16Le");
        break;
    case BuiltinTypeKind::U32:
        genGcSizedSer<isSerializer>("4", "Uint32Le");
        break;
    case BuiltinTypeKind::I32:
        genGcSizedSer<isSerializer>("4", "Int32Le");
        break;
    case BuiltinTypeKind::U64:
        genGcSizedSer<isSerializer>("8", "Uint64Le");
        break;
    case BuiltinTypeKind::I64:
        genGcSizedSer<isSerializer>("8", "Int64Le");
        break;
    case BuiltinTypeKind::F32:
        genGcSizedSer<isSerializer>("4", "Float32Le");
        break;
    case BuiltinTypeKind::F64:
        genGcSizedSer<isSerializer>("8", "Float64Le");
        break;
    case BuiltinTypeKind::Bool:
        genGcSizedSer<isSerializer>("1", "Uint8");
        break;
    case BuiltinTypeKind::Char:
        genGcSizedSer<isSerializer>("1", "Uint8");
        break;
    case BuiltinTypeKind::Varuint:
        genGcVarSer<isSerializer>("VarUint");
        break;
    case BuiltinTypeKind::Varint:
        genGcVarSer<isSerializer>("VarInt");
        break;
    case BuiltinTypeKind::Void:
        assert(false);
        break;
    default:
        assert(false);
    }
}

template <bool isSerializer>
void InlineTypeInspector::genGcSizedSer(bmcl::StringView sizeCheck, bmcl::StringView suffix)
{
    if (isSizeCheckEnabled()) {
        appendSizeCheck<false, isSerializer>(context(), sizeCheck, _output);
    }
    if (isSerializer) {
        _output->appendIndent(context());
        _output->append("dest->write");
        _output->append(suffix);
        _output->append('(');
        appendArgumentName();
        _output->append(");\n");
    } else {
        _output->appendIndent(context());
        appendArgumentName();
        _output->append(" = src->read");
        _output->append(suffix);
        _output->append('(');
        _output->append(");\n");
    }
}

template <bool isSerializer>
void InlineTypeInspector::genGcVarSer(bmcl::StringView suffix)
{
    _output->appendIndent(context());
    if (isSerializer) {
        _output->append("dest->write");
        _output->append(suffix);
        _output->append('(');
        appendArgumentName();
        _output->append(");\n");
    } else {
        _output->append("if (!src->read");
        _output->append(suffix);
        _output->append("(&");
        appendArgumentName();
        _output->append(")) {\n");
        _output->appendIndent(context());
        _output->append("    return false;\n");
        _output->appendIndent(context());
        _output->append("}\n");
    }
}

template <bool isSerializer>
void InlineTypeInspector::inspectOnboardBuiltin(const BuiltinType* type)
{
    switch (type->builtinTypeKind()) {
    case BuiltinTypeKind::USize:
        genOnboardSizedSer<isSerializer>("sizeof(void*)", "USizeLe");
        break;
    case BuiltinTypeKind::ISize:
        genOnboardSizedSer<isSerializer>("sizeof(void*)", "USizeLe");
        break;
    case BuiltinTypeKind::U8:
        genOnboardSizedSer<isSerializer>("sizeof(uint8_t)", "U8");
        break;
    case BuiltinTypeKind::I8:
        genOnboardSizedSer<isSerializer>("sizeof(uint8_t)", "U8");
        break;
    case BuiltinTypeKind::U16:
        genOnboardSizedSer<isSerializer>("sizeof(uint16_t)", "U16Le");
        break;
    case BuiltinTypeKind::I16:
        genOnboardSizedSer<isSerializer>("sizeof(uint16_t)", "U16Le");
        break;
    case BuiltinTypeKind::U32:
        genOnboardSizedSer<isSerializer>("sizeof(uint32_t)", "U32Le");
        break;
    case BuiltinTypeKind::I32:
        genOnboardSizedSer<isSerializer>("sizeof(uint32_t)", "U32Le");
        break;
    case BuiltinTypeKind::U64:
        genOnboardSizedSer<isSerializer>("sizeof(uint64_t)", "U64Le");
        break;
    case BuiltinTypeKind::I64:
        genOnboardSizedSer<isSerializer>("sizeof(uint64_t)", "U64Le");
        break;
    case BuiltinTypeKind::F32:
        genOnboardSizedSer<isSerializer>("sizeof(float)", "F32Le");
        break;
    case BuiltinTypeKind::F64:
        genOnboardSizedSer<isSerializer>("sizeof(double)", "F64Le");
        break;
    case BuiltinTypeKind::Bool:
        genOnboardSizedSer<isSerializer>("sizeof(uint8_t)", "U8");
        break;
    case BuiltinTypeKind::Char:
        genOnboardSizedSer<isSerializer>("sizeof(char)", "Char");
        break;
    case BuiltinTypeKind::Varuint:
        genOnboardVarSer<isSerializer>("Varuint");
        break;
    case BuiltinTypeKind::Varint:
        genOnboardVarSer<isSerializer>("Varint");
        break;
    case BuiltinTypeKind::Void:
        //TODO: disallow
        assert(false);
        break;
    default:
        assert(false);
    }
}

void InlineTypeInspector::deserializeOnboardPointer(const Type* type)
{
    if (isSizeCheckEnabled()) {
        appendSizeCheck<true, false>(context(), "sizeof(void*)", _output);
    }
    _output->appendIndent(context());
    appendArgumentName();
    _output->append(" = (");
    TypeReprGen reprGen(_output);
    reprGen.genOnboardTypeRepr(type);
    _output->append(")PhotonReader_ReadPtrLe(src);\n");
}

void InlineTypeInspector::serializeOnboardPointer(const Type* type)
{
    if (isSizeCheckEnabled()) {
        appendSizeCheck<true, true>(context(), "sizeof(void*)", _output);
    }
    _output->appendIndent(context());
    _output->append("PhotonWriter_WritePtrLe(dest, (const void*)");
    appendArgumentName();
    _output->append(");\n");
}

void InlineTypeInspector::deserializeGcPointer(const Type* type)
{
//TODO
}

void InlineTypeInspector::serializeGcPointer(const Type* type)
{
//TODO
}

template <bool isOnboard, bool isSerializer>
void InlineTypeInspector::inspectPointer(const Type* type)
{
    if (isOnboard) {
        if (isSerializer) {
            serializeOnboardPointer(type);
        } else {
            deserializeOnboardPointer(type);
        }
    } else {
        if (isSerializer) {
            serializeGcPointer(type);
        } else {
            deserializeGcPointer(type);
        }
    }
}

template <bool isSerializer>
void InlineTypeInspector::inspectOnboardNonInlineType(const Type* type)
{
    _output->appendIndent(context());
    _output->appendWithTryMacro([&, this, type](SrcBuilder* output) {
        TypeReprGen reprGen(_output);
        reprGen.genOnboardTypeRepr(type);
        if (isSerializer) {
            output->append("_Serialize(");
            if (type->typeKind() != TypeKind::Enum) {
                output->append('&');
            }
            appendArgumentName();
            output->append(", dest)");
        } else {
            output->append("_Deserialize(&");
            appendArgumentName();
            output->append(", src)");
        }
    });
}

template <bool isOnboard, bool isSerializer>
void InlineTypeInspector::inspectNonInlineType(const Type* type)
{
    if (isOnboard) {
        inspectOnboardNonInlineType<isSerializer>(type);
    } else {
        _output->appendIndent(context());
        _output->append("if (!");
        if (isSerializer) {
            _output->append("photongenSerialize");
        } else {
            _output->append("photongenDeserialize");
        }
        TypeNameGen gen(_output);
        gen.genTypeName(type);
        if (!isSerializer) {
            _output->append("(&");
            appendArgumentName();
            _output->append(", src, state)) {\n");
        } else {
            _output->append('(');
            appendArgumentName();
            _output->append(", dest, state)) {\n");
        }
        _output->appendIndent(context().indent());
        _output->append("return false;\n");
        _output->appendIndent(context());
        _output->append("}\n");
    }
}

template <bool isSerializer>
void InlineTypeInspector::genOnboardSizedSer(bmcl::StringView sizeCheck, bmcl::StringView suffix)
{
    if (isSerializer) {
        if (isSizeCheckEnabled()) {
            _output->appendWritableSizeCheck(context(), sizeCheck);
        }
        _output->appendIndent(context());
        _output->append("PhotonWriter_Write");
        _output->append(suffix);
        _output->append("(dest, ");
        appendArgumentName();
        _output->append(");\n");
    } else {
        if (isSizeCheckEnabled()) {
            _output->appendReadableSizeCheck(context(), sizeCheck);
        }
        _output->appendIndent(context());
        appendArgumentName();
        _output->append(" = PhotonReader_Read");
        _output->append(suffix);
        _output->append("(src);\n");
    }
}

template <bool isSerializer>
void InlineTypeInspector::genOnboardVarSer(bmcl::StringView suffix)
{
    _output->appendIndent(context());
    _output->appendWithTryMacro([&](SrcBuilder* output) {
        if (isSerializer) {
            output->append("PhotonWriter_Write");
            output->append(suffix);
            output->append("(dest, ");
            appendArgumentName();
            output->append(")");
        } else {
            output->append("PhotonReader_Read");
            output->append(suffix);
            output->append("(src, &");
            appendArgumentName();
            output->append(")");
        }
    });
}

template void InlineTypeInspector::inspect<true, true>(const Type* type, const InlineSerContext& ctx, bmcl::StringView argName, bool checkSizes);
template void InlineTypeInspector::inspect<true, false>(const Type* type, const InlineSerContext& ctx, bmcl::StringView argName, bool checkSizes);
template void InlineTypeInspector::inspect<false, true>(const Type* type, const InlineSerContext& ctx, bmcl::StringView argName, bool checkSizes);
template void InlineTypeInspector::inspect<false, false>(const Type* type, const InlineSerContext& ctx, bmcl::StringView argName, bool checkSizes);
template void InlineTypeInspector::appendSizeCheck<true, true>(const InlineSerContext& ctx, bmcl::StringView name, SrcBuilder* dest);
template void InlineTypeInspector::appendSizeCheck<true, false>(const InlineSerContext& ctx, bmcl::StringView name, SrcBuilder* dest);
template void InlineTypeInspector::appendSizeCheck<false, true>(const InlineSerContext& ctx, bmcl::StringView name, SrcBuilder* dest);
template void InlineTypeInspector::appendSizeCheck<false, false>(const InlineSerContext& ctx, bmcl::StringView name, SrcBuilder* dest);
}
