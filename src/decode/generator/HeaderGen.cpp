/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "decode/generator/HeaderGen.h"
#include "decode/core/Foreach.h"
#include "decode/core/HashSet.h"
#include "decode/ast/Component.h"
#include "decode/ast/Function.h"
#include "decode/generator/TypeNameGen.h"

namespace decode {

//TODO: refact

HeaderGen::HeaderGen(TypeReprGen* reprGen, SrcBuilder* output)
    : _output(output)
    , _typeDefGen(reprGen, output)
    , _prototypeGen(reprGen, output)
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
    appendImplBlockIncludes(namedType);
    _output->startCppGuard();
    appendFunctionPrototypes(namedType);
    if (type->typeKind() != TypeKind::Alias) {
        appendSerializerFuncPrototypes(namedType);
    }
    _output->endCppGuard();
    endIncludeGuard();
}

void HeaderGen::genComponentHeader(const Ast* ast, const Component* comp)
{
    _ast = ast;
    startIncludeGuard(comp);
    _output->appendLocalIncludePath("Config");
    _output->appendEol();

    _output->append("#define PHOTON_");
    _output->appendUpper(comp->name());
    _output->append("_COMPONENT_ID ");
    _output->appendNumericValue(comp->number());
    _output->appendEol();
    for (const StatusMsg* msg : comp->statusesRange()) {
        _output->append("#define PHOTON_");
        _output->appendUpper(comp->name());
        _output->append("_STATUS_");
        _output->appendUpper(msg->name());
        _output->append("_ID ");
        _output->appendNumericValue(msg->number());
        _output->appendEol();
    }
    for (const Command* func : comp->cmdsRange()) {
        _output->append("#define PHOTON_");
        _output->appendUpper(comp->name());
        _output->append("_CMD_");
        _output->appendUpper(func->name());
        _output->append("_ID ");
        _output->appendNumericValue(func->number());
        _output->appendEol();
    }
    _output->appendEol();

    appendIncludesAndFwds(comp);
    appendCommonIncludePaths();
    _typeDefGen.genComponentDef(comp);
    if (comp->hasParams()) {
        _output->append("extern Photon");
        _output->appendWithFirstUpper(comp->moduleName());
        _output->append(" _photon");
        _output->appendWithFirstUpper(comp->moduleName());
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

void HeaderGen::genDynArrayHeader(const DynArrayType* dynArray)
{
    _dynArrayName.clear();
    TypeNameGen gen(&_dynArrayName);
    gen.genTypeName(dynArray);
    startIncludeGuard(dynArray);
    _output->appendLocalIncludePath("Config");
    _output->appendEol();
    appendIncludesAndFwds(dynArray);
    appendCommonIncludePaths();
    _typeDefGen.genTypeDef(dynArray);
    appendImplBlockIncludes(dynArray);
    _output->startCppGuard();
    appendSerializerFuncPrototypes(dynArray);
    _output->endCppGuard();
    endIncludeGuard();
}

void HeaderGen::appendSerializerFuncPrototypes(const Component*)
{
}

void HeaderGen::appendSerializerFuncPrototypes(const Type* type)
{
    _prototypeGen.appendSerializerFuncDecl(type);
    _output->append(";\n");
    _prototypeGen.appendDeserializerFuncDecl(type);
    _output->append(";\n\n");
}

void HeaderGen::startIncludeGuard(bmcl::StringView modName, bmcl::StringView typeName)
{
    _output->startIncludeGuard(modName, typeName);
}

void HeaderGen::startIncludeGuard(const DynArrayType* dynArray)
{
    _output->startIncludeGuard("SLICE", _dynArrayName.view());
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

void HeaderGen::appendImplBlockIncludes(const DynArrayType* dynArray)
{
    _output->appendLocalIncludePath("core/Reader");
    _output->appendLocalIncludePath("core/Writer");
    _output->appendLocalIncludePath("core/Error");
    _output->appendEol();
}

void HeaderGen::appendImplBlockIncludes(const Component* comp)
{
    HashSet<std::string> dest;
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
    HashSet<std::string> dest;
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

void HeaderGen::appendIncludes(const HashSet<std::string>& src)
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
    HashSet<std::string> includePaths;
    _includeCollector.collect(comp, &includePaths);
    appendIncludes(includePaths);
}

void HeaderGen::appendIncludesAndFwds(const Type* topLevelType)
{
    HashSet<std::string> includePaths;
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
    case TypeKind::DynArray:
    case TypeKind::Struct:
    case TypeKind::Variant:
        return new ReferenceType(ReferenceKind::Pointer, false, type);
    case TypeKind::Imported:
        return wrapIntoPointerIfNeeded(type->asImported()->link());
    case TypeKind::Alias:
        return wrapIntoPointerIfNeeded(type->asAlias()->alias());
    case TypeKind::GenericParameter:
        return wrapIntoPointerIfNeeded(type->asGenericParemeter()->substitutedType());
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
            if (rv->isArray()) {
                _typeReprGen->genTypeRepr(rv.unwrap(), "rv");
            } else {
                Rc<const ReferenceType> rtype = new ReferenceType(ReferenceKind::Pointer, true, rv.unwrap());  //HACK
                _typeReprGen->genTypeRepr(rtype.get(), "rv"); //TODO: check name conflicts
            }
        }

        _output->append(");\n");
    }
    _output->appendEol();
}

void HeaderGen::appendFunctionPrototype(const Function* func, bmcl::StringView typeName)
{
    const FunctionType* type = func->type();
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
            _output->appendModPrefix();
            _output->append(typeName);
            _output->append(" self");
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
