#include "decode/generator/IncludeGen.h"
#include "decode/generator/SrcBuilder.h"
#include "decode/generator/TypeNameGen.h"
#include "decode/generator/TypeDependsCollector.h"
#include "decode/ast/Type.h"

namespace decode {

IncludeGen::IncludeGen(SrcBuilder* output)
    : _output(output)
{
}

void IncludeGen::appendExt()
{
    _output->append(_ext);
    _output->append("\"\n");
}

void IncludeGen::genNamedInclude(const NamedType* type)
{
    genNamedInclude(type, type);
}

void IncludeGen::genNamedInclude(const NamedType* type, const NamedType* origin)
{
    _output->append("#include \"");
    _output->append(_prefix);
    _output->append(origin->moduleName());
    _output->append('/');
    _output->appendWithFirstUpper(type->name());
    appendExt();
}

void IncludeGen::genOnboardDynArray(const DynArrayType* type)
{
    _output->append("#include \"");
    _output->append(_prefix);
    _output->append("_dynarray_/");
    TypeNameGen gen(_output);
    gen.genTypeName(type);
    appendExt();
}

void IncludeGen::genOnboardGenericInstantiation(const GenericInstantiationType* type)
{
    _output->append("#include \"");
    _output->append(_prefix);
    _output->append("_generic_/");
    if (type->moduleName() != "core") {
        _output->appendWithFirstUpper(type->moduleName());
    }
    _output->append(type->genericName());
    TypeNameGen gen(_output);
    for (const Type* t : type->substitutedTypesRange()) {
        gen.genTypeName(t);
    }
    appendExt();
}

template <bool isOnboard>
void IncludeGen::genIncludePaths(const HashSet<Rc<const Type>>* types)
{
    for (const Rc<const Type>& type : *types) {
    switch (type->typeKind()) {
        case TypeKind::Builtin:
            break;
        case TypeKind::Reference:
            break;
        case TypeKind::Array:
            break;
        case TypeKind::DynArray:
            if (isOnboard) {
                genOnboardDynArray(type->asDynArray());
            }
            break;
        case TypeKind::Function:
            break;
        case TypeKind::Enum:
            genNamedInclude(type->asEnum());
            break;
        case TypeKind::Struct:
            genNamedInclude(type->asStruct());
            break;
        case TypeKind::Variant:
            genNamedInclude(type->asVariant());
            break;
        case TypeKind::Imported:
            genNamedInclude(type->asImported(), type->asImported()->link());
            break;
        case TypeKind::Alias:
            genNamedInclude(type->asAlias());
            break;
        case TypeKind::Generic:
            if (!isOnboard) {
                genNamedInclude(type->asGeneric());
            }
            break;
        case TypeKind::GenericInstantiation:
            if (isOnboard) {
                genOnboardGenericInstantiation(type->asGenericInstantiation());
            } else {
                genOnboardGenericInstantiation(type->asGenericInstantiation());
                genNamedInclude(type->asGenericInstantiation()->genericType());
            }
            break;
        case TypeKind::GenericParameter:
            break;
        }
    }
}

void IncludeGen::genOnboardIncludePaths(const HashSet<Rc<const Type>>* types, bmcl::StringView ext)
{
    _ext = ext;
    _prefix = "photongen/onboard/";
    genIncludePaths<true>(types);
}

void IncludeGen::genGcIncludePaths(const HashSet<Rc<const Type>>* types, bmcl::StringView ext)
{
    _ext = ext;
    _prefix = "photongen/groundcontrol/";
    genIncludePaths<false>(types);
}

void IncludeGen::genGcIncludePaths(const Type* type, bmcl::StringView ext)
{
    TypeDependsCollector coll;
    TypeDependsCollector::Depends deps;
    coll.collect(type, &deps);
    genGcIncludePaths(&deps, ext);
    if (!deps.empty()) {
        _output->appendEol();
    }
}
}
