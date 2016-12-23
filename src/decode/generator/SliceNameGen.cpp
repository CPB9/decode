#include "decode/generator/SliceNameGen.h"
#include "decode/parser/Decl.h"

#include <bmcl/StringView.h>

namespace decode {

SliceNameGen::SliceNameGen(SrcBuilder* dest)
    : NameVisitor<SliceNameGen>(dest)
{
}

static bmcl::StringView builtinToName(const BuiltinType* type)
{
    switch (type->builtinTypeKind()) {
    case BuiltinTypeKind::USize:
        return "USize";
    case BuiltinTypeKind::ISize:
        return "ISize";
    case BuiltinTypeKind::Varuint:
        return "Varuint";
    case BuiltinTypeKind::Varint:
        return "Varint";
    case BuiltinTypeKind::U8:
        return "U8";
    case BuiltinTypeKind::I8:
        return "I8";
    case BuiltinTypeKind::U16:
        return "U16";
    case BuiltinTypeKind::I16:
        return "I16";
    case BuiltinTypeKind::U32:
        return "U32";
    case BuiltinTypeKind::I32:
        return "I32";
    case BuiltinTypeKind::U64:
        return "U64";
    case BuiltinTypeKind::I64:
        return "I64";
    case BuiltinTypeKind::Bool:
        return "Bool";
    case BuiltinTypeKind::Void:
        return "Void";
    case BuiltinTypeKind::Unknown:
        //FIXME: report error
        assert(false);
        return nullptr;
    }

    return nullptr;
}

inline bool SliceNameGen::visitBuiltinType(const BuiltinType* type)
{
    _output->append(builtinToName(type));
    return false;
}

inline bool SliceNameGen::visitArrayType(const ArrayType* type)
{
    _output->append("ArrOf");
    return true;
}

bool SliceNameGen::visitReferenceType(const ReferenceType* type)
{
    if (type->isMutable()) {
        _output->append("Mut");
    }
    switch (type->referenceKind()) {
    case ReferenceKind::Pointer:
        _output->append("PtrTo");
        break;
    case ReferenceKind::Reference:
        _output->append("RefTo");
        break;
    }
    return true;
}

inline bool SliceNameGen::visitSliceType(const SliceType* type)
{
    _output->append("SliceOf");
    return true;
}

inline bool SliceNameGen::appendTypeName(const Type* type)
{
    _output->appendWithFirstUpper(type->moduleName());
    _output->append(type->name());
    return false;
}

void SliceNameGen::genSliceName(const SliceType* type)
{
    _output->appendModPrefix();
    traverseType(type);
}
}
