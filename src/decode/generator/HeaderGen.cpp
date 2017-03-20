#include "decode/generator/HeaderGen.h"
#include "decode/core/Foreach.h"
#include "decode/parser/Component.h"
#include "decode/generator/TypeNameGen.h"

namespace decode {

//TODO: refact

HeaderGen::HeaderGen(const Rc<TypeReprGen>& reprGen, SrcBuilder* output)
    : _output(output)
    , _typeDefGen(reprGen, output)
    , _typeReprGen(reprGen)
{
}

HeaderGen::~HeaderGen()
{
}

void HeaderGen::genTypeHeader(const Ast* ast, const Type* type)
{
    const NamedType* namedType;
    switch (type->typeKind()) {
    case TypeKind::Enum:
    case TypeKind::Struct:
    case TypeKind::Variant:
    case TypeKind::Alias:
        namedType = static_cast<const NamedType*>(type);
        break;
    default:
        return;
    }
    _ast = ast;
    startIncludeGuard(namedType);
    _output->appendLocalIncludePath("Config");
    _output->appendEol();
    appendIncludesAndFwds(type);
    appendCommonIncludePaths();
    _typeDefGen.genTypeDef(type);
    if (type->typeKind() != TypeKind::Alias) {
        appendImplBlockIncludes(namedType);
        _output->startCppGuard();
        appendFunctionPrototypes(namedType);
        appendSerializerFuncPrototypes(namedType);
        _output->endCppGuard();
    }
    endIncludeGuard();
}

void HeaderGen::genComponentHeader(const Ast* ast, const Component* comp)
{
    _ast = ast;
    startIncludeGuard(comp);
    _output->appendLocalIncludePath("Config");
    _output->appendEol();
    appendIncludesAndFwds(comp);
    appendCommonIncludePaths();
    _typeDefGen.genComponentDef(comp);
    if (comp->parameters().isSome()) {
        _output->append("extern Photon");
        _output->appendWithFirstUpper(comp->moduleName());
        _output->append(" _");
        _output->append(comp->moduleName());
        _output->append(";\n\n");
    }
    appendImplBlockIncludes(comp);
    _output->startCppGuard();
    appendFunctionPrototypes(comp);
    appendCommandPrototypes(comp);
    appendSerializerFuncPrototypes(comp);
    _output->endCppGuard();
    endIncludeGuard();
}

void HeaderGen::genSliceHeader(const SliceType* slice)
{
    _sliceName.clear();
    TypeNameGen gen(&_sliceName);
    gen.genTypeName(slice);
    startIncludeGuard(slice);
    _output->appendLocalIncludePath("Config");
    _output->appendEol();
    appendIncludesAndFwds(slice);
    appendCommonIncludePaths();
    _typeDefGen.genTypeDef(slice);
    appendImplBlockIncludes(slice);
    _output->startCppGuard();
    appendSerializerFuncPrototypes(slice);
    _output->endCppGuard();
    endIncludeGuard();
}

void HeaderGen::appendSerializerFuncPrototypes(const Component*)
{
}

void HeaderGen::appendSerializerFuncPrototypes(const Type* type)
{
    appendSerializerFuncDecl(type);
    _output->append(";\n");
    appendDeserializerFuncDecl(type);
    _output->append(";\n\n");
}

void HeaderGen::startIncludeGuard(bmcl::StringView modName, bmcl::StringView typeName)
{
    _output->startIncludeGuard(modName, typeName);
}

void HeaderGen::startIncludeGuard(const SliceType* slice)
{
    _output->startIncludeGuard("SLICE", _sliceName.view());
}

void HeaderGen::startIncludeGuard(const Component* comp)
{
    _output->startIncludeGuard("COMPONENT", comp->moduleName());
}

void HeaderGen::startIncludeGuard(const NamedType* type)
{
    _output->startIncludeGuard(type->moduleName(), type->name());
}

void HeaderGen::endIncludeGuard()
{
    _output->endIncludeGuard();
}

void HeaderGen::appendImplBlockIncludes(const SliceType* slice)
{
    _output->appendLocalIncludePath("core/Reader");
    _output->appendLocalIncludePath("core/Writer");
    _output->appendLocalIncludePath("core/Error");
    _output->appendEol();
}

void HeaderGen::appendImplBlockIncludes(const Component* comp)
{
    std::unordered_set<std::string> dest;
    if (comp->commands().isSome()) {
        for (const Rc<Function>& fn : comp->commands().unwrap()->functions()) {
            _includeCollector.collect(fn->type(), &dest);
        }
    }
    if (comp->implBlock().isSome()) {
        for (const Rc<Function>& fn : comp->implBlock().unwrap()->functions()) {
            _includeCollector.collect(fn->type(), &dest);
        }
    }
    dest.insert("core/Reader");
    dest.insert("core/Writer");
    dest.insert("core/Error");
    appendIncludes(dest);
}

void HeaderGen::appendImplBlockIncludes(const NamedType* topLevelType)
{
    bmcl::Option<const Rc<ImplBlock>&> impl = _ast->findImplBlockWithName(topLevelType->name());
    std::unordered_set<std::string> dest;
    if (impl.isSome()) {
        for (const Rc<Function>& fn : impl.unwrap()->functions()) {
            _includeCollector.collect(fn->type(), &dest);
        }
    }
    dest.insert("core/Reader");
    dest.insert("core/Writer");
    dest.insert("core/Error");
    appendIncludes(dest);
}

void HeaderGen::appendIncludes(const std::unordered_set<std::string>& src)
{
    for (const std::string& path : src) {
        _output->appendLocalIncludePath(path);
    }

    if (!src.empty()) {
        _output->appendEol();
    }
}

void HeaderGen::appendIncludesAndFwds(const Component* comp)
{
    std::unordered_set<std::string> includePaths;
    _includeCollector.collect(comp, &includePaths);
    appendIncludes(includePaths);
}

void HeaderGen::appendIncludesAndFwds(const Type* topLevelType)
{
    std::unordered_set<std::string> includePaths;
    _includeCollector.collect(topLevelType, &includePaths);
    appendIncludes(includePaths);
}

void HeaderGen::appendFunctionPrototypes(const Component* comp)
{
    if (comp->implBlock().isNone()) {
        return;
    }
    appendFunctionPrototypes(comp->implBlock().unwrap()->functions(), bmcl::StringView::empty());
}

void HeaderGen::appendFunctionPrototypes(const std::vector<Rc<Function>>& funcs, bmcl::StringView typeName)
{
    for (const Rc<Function>& func : funcs) {
        appendFunctionPrototype(func.get(), typeName);
    }
    if (!funcs.empty()) {
        _output->append('\n');
    }
}

void HeaderGen::appendFunctionPrototypes(const NamedType* type)
{
    bmcl::Option<const Rc<ImplBlock>&> block = _ast->findImplBlockWithName(type->name());
    if (block.isNone()) {
        return;
    }
    appendFunctionPrototypes(block.unwrap()->functions(), type->name());
}

static Rc<Type> wrapIntoPointerIfNeeded(Type* type)
{
    switch (type->typeKind()) {
    case TypeKind::Reference:
    case TypeKind::Array:
    case TypeKind::Function:
    case TypeKind::Enum:
    case TypeKind::Builtin:
        return type;
    case TypeKind::Slice:
    case TypeKind::Struct:
    case TypeKind::Variant:
        return new ReferenceType(ReferenceKind::Pointer, false, type);
    case TypeKind::Imported:
        return wrapIntoPointerIfNeeded(type->asImported()->link());
    case TypeKind::Alias:
        return wrapIntoPointerIfNeeded(type->asAlias()->alias());
    }
    assert(false);
    return nullptr;
}

void HeaderGen::appendCommandPrototypes(const Component* comp)
{
    if (comp->commands().isNone()) {
        return;
    }
    for (const Rc<Function>& func : comp->commands().unwrap()->functions()) {
        const FunctionType* ftype = func->type();
        _output->append("PhotonError Photon");
        _output->appendWithFirstUpper(comp->moduleName());
        _output->append("_");
        _output->appendWithFirstUpper(func->name());
        _output->append("(");

        foreachList(ftype->arguments(), [this](const Rc<Field>& field) {
            Rc<Type> type = wrapIntoPointerIfNeeded(field->type());
            _typeReprGen->genTypeRepr(type.get(), field->name());
        }, [this](const Rc<Field>&) {
            _output->append(", ");
        });

        if (ftype->returnValue().isSome()) {
            if (!ftype->arguments().empty()) {
                _output->append(", ");
            }
            ReferenceType* rtype = new ReferenceType(ReferenceKind::Pointer, true, ftype->returnValue().unwrap().get());
            _typeReprGen->genTypeRepr(rtype, "rv"); //TODO: check name conflicts
        }

        _output->append(");\n");
    }
    _output->appendEol();
}

void HeaderGen::appendFunctionPrototype(const Function* func, bmcl::StringView typeName)
{
    const FunctionType* type = func->type();
    if (type->returnValue().isSome()) {
        _typeReprGen->genTypeRepr(type->returnValue()->get());
        _output->append(' ');
    } else {
        _output->append("void ");
    }
    _output->appendModPrefix();
    _output->append(typeName);
    _output->append('_');
    _output->appendWithFirstUpper(func->name());
    _output->append('(');

    if (type->selfArgument().isSome()) {
        SelfArgument self = type->selfArgument().unwrap();
        switch(self) {
        case SelfArgument::Reference:
            _output->append("const ");
        case SelfArgument::MutReference:
            _output->appendModPrefix();
            _output->append(typeName);
            _output->append("* self");
            break;
        case SelfArgument::Value:
            break;
        }
        if (!type->arguments().empty()) {
            _output->append(", ");
        }
    }

    if (type->arguments().size() > 0) {
        for (auto it = type->arguments().begin(); it < (type->arguments().end() - 1); it++) {
            const Field* field = it->get();
            _typeReprGen->genTypeRepr(field->type(), field->name());
            _output->append(", ");
        }
        _typeReprGen->genTypeRepr(type->arguments().back()->type(), type->arguments().back()->name());
    }

    _output->append(");\n");
}

void HeaderGen::appendCommonIncludePaths()
{
    _output->appendInclude("stdbool.h");
    _output->appendInclude("stddef.h");
    _output->appendInclude("stdint.h");
    _output->appendEol();
}
}
