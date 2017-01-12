#include "decode/generator/HeaderGen.h"
#include "decode/parser/Component.h"
#include "decode/generator/TypeNameGen.h"

namespace decode {

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
    appendImplBlockIncludes(namedType);
    appendFunctionPrototypes(namedType);
    appendSerializerFuncPrototypes(namedType);
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
    appendImplBlockIncludes(comp);
    appendFunctionPrototypes(comp);
    appendSerializerFuncPrototypes(comp);
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
    appendSerializerFuncPrototypes(slice);
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
    auto writeGuardMacro = [this, typeName, modName]() {
        _output->append("__PHOTON_");
        _output->append(modName.toUpper());
        _output->append('_');
        _output->append(typeName.toUpper()); //FIXME
        _output->append("_H__\n");
    };
    _output->append("#ifndef ");
    writeGuardMacro();
    _output->append("#define ");
    writeGuardMacro();
    _output->appendEol();
}

void HeaderGen::startIncludeGuard(const SliceType* slice)
{
    startIncludeGuard("SLICE", _sliceName.view());
}

void HeaderGen::startIncludeGuard(const Component* comp)
{
    startIncludeGuard("COMPONENT", comp->moduleName());
}

void HeaderGen::startIncludeGuard(const NamedType* type)
{
    startIncludeGuard(type->moduleName(), type->name());
}

void HeaderGen::endIncludeGuard()
{
    _output->append("#endif\n");
    _output->appendEol();
}

void HeaderGen::appendImplBlockIncludes(const Component* comp)
{
    std::unordered_set<std::string> dest;
    if (comp->commands().isSome()) {
    for (const Rc<FunctionType>& fn : comp->commands().unwrap()->functions()) {
        _includeCollector.collectIncludesAndFwdsForType(fn.get(), &dest);
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
        for (const Rc<FunctionType>& fn : impl.unwrap()->functions()) {
            _includeCollector.collectIncludesAndFwdsForType(fn.get(), &dest);
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
    if (comp->parameters().isNone()) {
        return;
    }
    std::unordered_set<std::string> includePaths;
    for (const Rc<Field>& field : *comp->parameters().unwrap()->fields()) {
        _includeCollector.collectIncludesAndFwdsForType(field->type().get(), &includePaths);
    }
    appendIncludes(includePaths);
}

void HeaderGen::appendIncludesAndFwds(const Type* topLevelType)
{
    std::unordered_set<std::string> includePaths;
    _includeCollector.collectIncludesAndFwdsForType(topLevelType, &includePaths);
    appendIncludes(includePaths);
}

void HeaderGen::appendFunctionPrototypes(const Component* comp)
{
    if (comp->commands().isNone()) {
        return;
    }
    appendFunctionPrototypes(comp->commands().unwrap()->functions(), bmcl::StringView::empty());
}

void HeaderGen::appendFunctionPrototypes(const std::vector<Rc<FunctionType>>& funcs, bmcl::StringView typeName)
{
    for (const Rc<FunctionType>& func : funcs) {
        appendFunctionPrototype(func, typeName);
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

void HeaderGen::appendFunctionPrototype(const Rc<FunctionType>& func, bmcl::StringView typeName)
{
    if (func->returnValue().isSome()) {
        _typeReprGen->genTypeRepr(func->returnValue()->get());
        _output->append(' ');
    } else {
        _output->append("void ");
    }
    _output->appendModPrefix();
    _output->append(typeName);
    _output->append('_');
    _output->appendWithFirstUpper(func->name());
    _output->append('(');

    if (func->selfArgument().isSome()) {
        SelfArgument self = func->selfArgument().unwrap();
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
        if (!func->arguments().empty()) {
            _output->append(", ");
        }
    }

    if (func->arguments().size() > 0) {
        for (auto it = func->arguments().begin(); it < (func->arguments().end() - 1); it++) {
            const Field* field = it->get();
            _typeReprGen->genTypeRepr(field->type().get(), field->name());
            _output->append(", ");
        }
        _typeReprGen->genTypeRepr(func->arguments().back()->type().get(), func->arguments().back()->name());
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
