#include "decode/generator/Utils.h"
#include "decode/ast/Type.h"
#include "decode/core/StringBuilder.h"

#include <bmcl/StringView.h>

namespace decode {

Rc<Type> wrapPassedTypeIntoPointerIfRequired(Type* type)
{
    switch (type->typeKind()) {
    case TypeKind::Reference:
    case TypeKind::Array:
    case TypeKind::Function:
    case TypeKind::Enum:
    case TypeKind::Builtin:
        return type;
    case TypeKind::DynArray:
    case TypeKind::Struct:
    case TypeKind::Variant:
    case TypeKind::GenericInstantiation:
        return new ReferenceType(ReferenceKind::Pointer, false, type);
    case TypeKind::Imported:
        return wrapPassedTypeIntoPointerIfRequired(type->asImported()->link());
    case TypeKind::Alias:
        return wrapPassedTypeIntoPointerIfRequired(type->asAlias()->alias());
    case TypeKind::Generic:
        assert(false);
        return nullptr;
    case TypeKind::GenericParameter:
        assert(false);
        return nullptr;
    }
    assert(false);
    return nullptr;
}

void derefPassedVarNameIfRequired(const Type* type, bmcl::StringView name, StringBuilder* dest)
{
    switch (type->typeKind()) {
    case TypeKind::Reference:
    case TypeKind::Array:
    case TypeKind::Function:
    case TypeKind::Enum:
    case TypeKind::Builtin:
        dest->append(name);
        return;
    case TypeKind::DynArray:
    case TypeKind::Struct:
    case TypeKind::Variant:
    case TypeKind::GenericInstantiation:
        dest->prepend("(*");
        dest->append(name);
        dest->append(")");
        return;
    case TypeKind::Imported:
        derefPassedVarNameIfRequired(type->asImported()->link(), name, dest);
        return;
    case TypeKind::Alias:
        derefPassedVarNameIfRequired(type->asAlias()->alias(), name, dest);
        return;
    case TypeKind::Generic:
        assert(false);
        return;
    case TypeKind::GenericParameter:
        assert(false);
        return;
    }
    assert(false);
}
}
