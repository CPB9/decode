#include "decode/generator/HeaderGen.h"
#include "decode/core/Foreach.h"
#include "decode/parser/Component.h"
#include "decode/generator/TypeNameGen.h"

namespace decode {

//TODO: refact

HeaderGen::HeaderGen(TypeReprGen* reprGen, SrcBuilder* output)
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
    if (comp->hasParams()) {
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
    for (const Function* fn : comp->cmdsRange()) {
        _includeCollector.collect(fn->type(), &dest);
    }
    bmcl::OptionPtr<const ImplBlock> block = comp->implBlock();
    if (block.isSome()) {
        for (const Function* fn : block.unwrap()->functionsRange()) {
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
    bmcl::OptionPtr<const ImplBlock> impl = _ast->findImplBlockWithName(topLevelType->name());
    std::unordered_set<std::string> dest;
    if (impl.isSome()) {
        for (const Function* fn : impl->functionsRange()) {
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
    bmcl::OptionPtr<const ImplBlock> impl = comp->implBlock();
    if (impl.isNone()) {
        return;
    }
    appendFunctionPrototypes(impl.unwrap()->functionsRange(), bmcl::StringView::empty());
}

void HeaderGen::appendFunctionPrototypes(RcVec<Function>::ConstRange funcs, bmcl::StringView typeName)
{
    for (const Function* func : funcs) {
        appendFunctionPrototype(func, typeName);
    }
    if (!funcs.isEmpty()) {
        _output->append('\n');
    }
}

void HeaderGen::appendFunctionPrototypes(const NamedType* type)
{
    bmcl::OptionPtr<const ImplBlock> block = _ast->findImplBlockWithName(type->name());
    if (block.isNone()) {
        return;
    }
    appendFunctionPrototypes(block.unwrap()->functionsRange(), type->name());
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
    if (!comp->hasCmds()) {
        return;
    }
    for (const Function* func : comp->cmdsRange()) {
        const FunctionType* ftype = func->type();
        _output->append("PhotonError Photon");
        _output->appendWithFirstUpper(comp->moduleName());
        _output->append("_");
        _output->appendWithFirstUpper(func->name());
        _output->append("(");

        foreachList(ftype->argumentsRange(), [this](const Field* field) {
            Rc<Type> type = wrapIntoPointerIfNeeded(const_cast<Type*>(field->type())); //HACK
            _typeReprGen->genTypeRepr(type.get(), field->name());
        }, [this](const Field*) {
            _output->append(", ");
        });
        auto rv = const_cast<FunctionType*>(ftype)->returnValue(); //HACK
        if (rv.isSome()) {
            if (ftype->hasArguments()) {
                _output->append(", ");
            }
            Rc<const ReferenceType> rtype = new ReferenceType(ReferenceKind::Pointer, true, rv.unwrap());  //HACK
            _typeReprGen->genTypeRepr(rtype.get(), "rv"); //TODO: check name conflicts
        }

        _output->append(");\n");
    }
    _output->appendEol();
}

void HeaderGen::appendFunctionPrototype(const Function* func, bmcl::StringView typeName)
{
    const FunctionType* type = func->type();
    if (func->name() == "isAtEnd") {
        BMCL_DEBUG() << "aaaaaaaaaaaaaaaaaaaaaaaaaa " << type->hasReturnValue();
    }
    bmcl::OptionPtr<const Type> rv = type->returnValue();
    if (rv.isSome()) {
        _typeReprGen->genTypeRepr(rv.unwrap());
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
        if (type->hasArguments()) {
            _output->append(", ");
        }
    }

    foreachList(type->argumentsRange(), [this](const Field* field) {
        _typeReprGen->genTypeRepr(field->type(), field->name());
    }, [this](const Field*) {
        _output->append(", ");
    });

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
