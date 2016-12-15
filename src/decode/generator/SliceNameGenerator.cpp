#include "decode/generator/SliceNameGenerator.h"
#include "decode/parser/Decl.h"

#include <bmcl/StringView.h>

namespace decode {

SliceNameGenerator::SliceNameGenerator(SrcBuilder* dest)
    : NameVisitor<SliceNameGenerator>(dest)
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

inline bool SliceNameGenerator::visitBuiltin(const BuiltinType* type)
{
    _output->append(builtinToName(type));
    return false;
}

inline bool SliceNameGenerator::visitArray(const ArrayType* type)
{
    _output->append("ArrOf");
    return true;
}

bool SliceNameGenerator::visitReference(const ReferenceType* type)
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

inline bool SliceNameGenerator::visitSlice(const SliceType* type)
{
    _output->append("SliceOf");
    return true;
}

inline bool SliceNameGenerator::appendTypeName(const Type* type)
{
    _output->appendWithFirstUpper(type->moduleName());
    _output->append(type->name());
    return false;
}

void SliceNameGenerator::genSliceName(const SliceType* type)
{
    _output->appendModPrefix();
    traverseType(type);
}
}
