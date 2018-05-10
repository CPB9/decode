/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "decode/generator/OnboardTypeHeaderGen.h"
#include "decode/core/Foreach.h"
#include "decode/core/HashSet.h"
#include "decode/core/EncodedSizes.h"
#include "decode/ast/Component.h"
#include "decode/ast/Function.h"
#include "decode/ast/AstVisitor.h"
#include "decode/generator/TypeNameGen.h"
#include "decode/generator/IncludeGen.h"

#include <bmcl/Logging.h>

namespace decode {

//TODO: refact

OnboardTypeHeaderGen::OnboardTypeHeaderGen(SrcBuilder* output)
    : _output(output)
    , _typeDefGen(output)
    , _prototypeGen(output)
{
}

OnboardTypeHeaderGen::~OnboardTypeHeaderGen()
{
}

void OnboardTypeHeaderGen::genTypeHeader(const Ast* ast, const TopLevelType* type, bmcl::StringView name)
{
    switch (type->typeKind()) {
    case TypeKind::Enum:
    case TypeKind::Struct:
    case TypeKind::Variant:
    case TypeKind::Alias:
    case TypeKind::GenericInstantiation:
        break;
    default:
        return;
    }
    _ast = ast;
    startIncludeGuard(type, name);
    _output->appendOnboardIncludePath("Config");
    _output->appendEol();
    appendIncludesAndFwds(type);
    appendCommonIncludePaths();
    _typeDefGen.genTypeDef(type, name);
    if (type->isGenericInstantiation()) {
        appendImplBlockIncludes(type->asGenericInstantiation()->genericType(), name);
    } else {
        appendImplBlockIncludes(type, name);
    }
    _output->startCppGuard();
    appendFunctionPrototypes(type, name);
    if (type->typeKind() != TypeKind::Alias) {
        appendSerializerFuncPrototypes(type);
    }
    appendMinMaxSizeFuncs(type, name);
    _output->endCppGuard();
    endIncludeGuard();
}

void OnboardTypeHeaderGen::appendMinMaxSizeFuncs(const Type* type, bmcl::StringView name)
{
    EncodedSizes sizes = type->encodedSizes();
    appendSizeFuncs(type, name, "Min", sizes.min);
    appendSizeFuncs(type, name, "Max", sizes.max);
    _output->appendEol();
}

void OnboardTypeHeaderGen::appendSizeFuncs(const Type* type, bmcl::StringView name, bmcl::StringView prefix, std::size_t size)
{
    (void)type;
    _output->append("static inline size_t Photon");
    _output->appendWithFirstUpper(name);
    _output->append("_");
    _output->append(prefix);
    _output->append("EncodedSize() { return ");
    _output->appendNumericValue(size);
    _output->append("; }\n");
}

template <typename T>
void OnboardTypeHeaderGen::appendComponentVarDefine(const Component* comp, const T* value, bmcl::StringView sep)
{
    _output->append("#define PHOTON_");
    _output->appendUpper(comp->name());
    _output->append(sep);
    _output->appendUpper(value->name());
    _output->append("_ID ");
    _output->appendNumericValue(value->number());
    _output->appendEol();
}

void OnboardTypeHeaderGen::genComponentHeader(const Ast* ast, const Component* comp)
{
    _ast = ast;
    startIncludeGuard(comp);
    _output->appendOnboardIncludePath("Config");
    _output->appendEol();

    appendIncludesAndFwds(comp);
    appendCommonIncludePaths();

    _output->append("/*component id*/\n");
    _output->append("#define PHOTON_");
    _output->appendUpper(comp->name());
    _output->append("_COMPONENT_ID ");
    _output->appendNumericValue(comp->number());
    _output->append("\n\n");

    _output->append("/*cmd ids*/\n");
    for (const Command* func : comp->cmdsRange()) {
        appendComponentVarDefine(comp, func, "_CMD_");
    }
    _output->appendEol();
    _output->append("/*status ids*/\n");
    for (const StatusMsg* msg : comp->statusesRange()) {
        appendComponentVarDefine(comp, msg, "_STATUS_");
    }
    _output->appendEol();
    _output->append("/*event ids*/\n");
    for (const EventMsg* msg : comp->eventsRange()) {
        appendComponentVarDefine(comp, msg, "_EVENT_");
    }
    _output->appendEol();

    _output->append("/*component state*/\n");
    _typeDefGen.genComponentDef(comp);

    appendImplBlockIncludes(comp);
    _output->startCppGuard();

    if (comp->hasVars()) {
        _output->append("extern Photon");
        _output->appendWithFirstUpper(comp->moduleName());
        _output->append(" _photon");
        _output->appendWithFirstUpper(comp->moduleName());
        _output->append(";\n\n");
    }

    _output->append("/**********************************IMPLEMENT***********************************/\n\n");
    appendImplPrototypes(comp);
    appendCommandPrototypes(comp);
    appendCommandArgAllocators(comp);
    _output->append("/******************************************************************************/\n\n");

    appendCmdEncoderPrototypes(comp);
    appendCmdDecoderPrototypes(comp);
    appendCmdMinMaxSizeFuncs(comp);
    appendStatusEncoderPrototypes(comp);
    appendStatusStructs(comp);
    appendStatusDecoderPrototypes(comp);
    appendStatusMinMaxSizeFuncs(comp);
    appendEventSenderPrototypes(comp);
    appendEventStructs(comp);
    appendEventDecoderPrototypes(comp);
    appendEventMinMaxSizeFuncs(comp);
    _output->endCppGuard();
    endIncludeGuard();
}

void OnboardTypeHeaderGen::appendStatusStructs(const Component* comp)
{
    TypeReprGen reprGen(_output);
    StringBuilder fieldName;
    fieldName.reserve(31);
    _output->append("/*statuses*/\n");
    for (const StatusMsg* msg : comp->statusesRange()) {
        _output->append("typedef struct {\n");

        for (const VarRegexp* part : msg->partsRange()) {
            _output->appendIndent();
            part->buildFieldName(&fieldName);
            reprGen.genOnboardTypeRepr(part->type(), fieldName.view());
            _output->append(";\n");
            fieldName.clear();
        }

        _output->append("} Photon");
        _output->appendWithFirstUpper(comp->moduleName());
        _output->append("_StatusMsg_");
        _output->appendWithFirstUpper(msg->name());
        _output->append(";\n\n");
    }
}

void OnboardTypeHeaderGen::appendEventStructs(const Component* comp)
{
    TypeReprGen reprGen(_output);
    _output->append("/*events*/\n");
    for (const EventMsg* msg : comp->eventsRange()) {
        if (msg->partsRange().empty()) {
            continue;
        }
        _output->append("typedef struct {\n");
        for (const Field* field : msg->partsRange()) {
            _output->appendIndent();
            reprGen.genOnboardTypeRepr(field->type(), field->name());
            _output->append(";\n");
        }
        _output->append("} Photon");
        _output->appendWithFirstUpper(comp->moduleName());
        _output->append("_EventMsg_");
        _output->appendWithFirstUpper(msg->name());
        _output->append(";\n\n");
    }
}

void OnboardTypeHeaderGen::genDynArrayHeader(const DynArrayType* dynArray)
{
    _dynArrayName.clear();
    TypeNameGen gen(&_dynArrayName);
    gen.genTypeName(dynArray);
    startIncludeGuard(dynArray);
    _output->appendOnboardIncludePath("Config");
    _output->appendEol();
    appendIncludesAndFwds(dynArray);
    appendCommonIncludePaths();
    _typeDefGen.genTypeDef(dynArray);
    appendImplBlockIncludes(dynArray);
    _output->startCppGuard();
    appendSerializerFuncPrototypes(dynArray);
    appendMinMaxSizeFuncs(dynArray, _dynArrayName.view());
    _output->endCppGuard();
    endIncludeGuard();
}

void OnboardTypeHeaderGen::appendSerializerFuncPrototypes(const Component*)
{
}

void OnboardTypeHeaderGen::appendSerializerFuncPrototypes(const Type* type)
{
    _prototypeGen.appendTypeSerializerFunctionPrototype(type);
    _output->append(";\n");
    _prototypeGen.appendTypeDeserializerFunctionPrototype(type);
    _output->append(";\n\n");
}

void OnboardTypeHeaderGen::startIncludeGuard(bmcl::StringView modName, bmcl::StringView typeName)
{
    _output->startIncludeGuard(modName, typeName);
}

void OnboardTypeHeaderGen::startIncludeGuard(const DynArrayType* dynArray)
{
    _output->startIncludeGuard("SLICE", _dynArrayName.view());
}

void OnboardTypeHeaderGen::startIncludeGuard(const Component* comp)
{
    _output->startIncludeGuard("COMPONENT", comp->moduleName());
}

void OnboardTypeHeaderGen::startIncludeGuard(const TopLevelType* type, bmcl::StringView name)
{
    _output->startIncludeGuard(type->moduleName(), name);
}

void OnboardTypeHeaderGen::startIncludeGuard(const NamedType* type)
{
    _output->startIncludeGuard(type->moduleName(), type->name());
}

void OnboardTypeHeaderGen::endIncludeGuard()
{
    _output->endIncludeGuard();
}

void OnboardTypeHeaderGen::appendImplBlockIncludes(const DynArrayType* dynArray)
{
    _output->appendOnboardIncludePath("core/Reader");
    _output->appendOnboardIncludePath("core/Writer");
    _output->appendOnboardIncludePath("core/Error");
    _output->appendEol();
}

void OnboardTypeHeaderGen::appendImplBlockIncludes(const Component* comp)
{
    _output->appendOnboardIncludePath("core/Reader");
    _output->appendOnboardIncludePath("core/Writer");
    _output->appendOnboardIncludePath("core/Error");
    _output->appendEol();

    TypeDependsCollector::Depends dest;
    for (const Command* cmd : comp->cmdsRange()) {
        _includeCollector.collect(cmd->type(), &dest);
    }
    for (const StatusMsg* msg : comp->statusesRange()) {
        _includeCollector.collect(msg, &dest);
    }
    for (const EventMsg* msg : comp->eventsRange()) {
        _includeCollector.collect(msg, &dest);
    }
    bmcl::OptionPtr<const ImplBlock> block = comp->implBlock();
    if (block.isSome()) {
        for (const Function* fn : block.unwrap()->functionsRange()) {
            _includeCollector.collect(fn->type(), &dest);
        }
    }
    appendIncludes(dest);
}

void OnboardTypeHeaderGen::appendImplBlockIncludes(const TopLevelType* topLevelType, bmcl::StringView name)
{
    _output->appendOnboardIncludePath("core/Reader");
    _output->appendOnboardIncludePath("core/Writer");
    _output->appendOnboardIncludePath("core/Error");
    _output->appendEol();

    bmcl::OptionPtr<const ImplBlock> impl = _ast->findImplBlock(topLevelType);
    TypeDependsCollector::Depends dest;
    if (impl.isSome()) {
        for (const Function* fn : impl->functionsRange()) {
            _includeCollector.collect(fn->type(), &dest);
        }
    }
    appendIncludes(dest);
}

void OnboardTypeHeaderGen::appendImplBlockIncludes(const NamedType* topLevelType)
{
    appendImplBlockIncludes(topLevelType, topLevelType->name());
}

void OnboardTypeHeaderGen::appendIncludes(const TypeDependsCollector::Depends& src)
{
    IncludeGen gen(_output);
    gen.genOnboardIncludePaths(&src);

    if (!src.empty()) {
        _output->appendEol();
    }
}

void OnboardTypeHeaderGen::appendIncludesAndFwds(const Component* comp)
{
    TypeDependsCollector::Depends includePaths;
    _includeCollector.collect(comp, &includePaths);
    appendIncludes(includePaths);
}

void OnboardTypeHeaderGen::appendIncludesAndFwds(const Type* topLevelType)
{
    TypeDependsCollector::Depends includePaths;
    _includeCollector.collect(topLevelType, &includePaths);
    appendIncludes(includePaths);
}

void OnboardTypeHeaderGen::appendImplPrototypes(const Component* comp)
{
    _output->append("/*impl*/\n");
    bmcl::OptionPtr<const ImplBlock> impl = comp->implBlock();
    if (impl.isNone()) {
        return;
    }
    appendFunctionPrototypes(impl.unwrap()->functionsRange(), comp->name());
}

void OnboardTypeHeaderGen::appendFunctionPrototypes(RcVec<Function>::ConstRange funcs, bmcl::StringView typeName)
{
    for (const Function* func : funcs) {
        appendFunctionPrototype(func, typeName);
    }
    if (!funcs.isEmpty()) {
        _output->append('\n');
    }
}

void OnboardTypeHeaderGen::appendFunctionPrototypes(const TopLevelType* type, bmcl::StringView name)
{
    bmcl::OptionPtr<const ImplBlock> block = _ast->findImplBlock(type);
    if (block.isNone()) {
        return;
    }
    appendFunctionPrototypes(block.unwrap()->functionsRange(), name);
}

void OnboardTypeHeaderGen::appendFunctionPrototypes(const NamedType* type)
{
    appendFunctionPrototypes(type, type->name());
}

void OnboardTypeHeaderGen::appendCmdEncoderPrototypes(const Component* comp)
{
    _output->append("/*cmd encoders*/\n");
    TypeReprGen reprGen(_output);
    for (const Command* cmd : comp->cmdsRange()) {
        _prototypeGen.appendCmdEncoderFunctionPrototype(comp, cmd, &reprGen);
        _output->append(";\n");
    }
    _output->appendEol();
}

void OnboardTypeHeaderGen::appendCmdDecoderPrototypes(const Component* comp)
{
    _output->append("/*cmd decoders*/\n");
    for (const Command* cmd : comp->cmdsRange()) {
        _prototypeGen.appendCmdDecoderFunctionPrototype(comp, cmd);
        _output->append(";\n");
    }
    _output->appendEol();
}

void OnboardTypeHeaderGen::appendCmdMinMaxSizeFuncs(const Component* comp)
{
    _output->append("/*cmd sizes*/\n");
    for (const Command* cmd : comp->cmdsRange()) {
        EncodedSizes sizes = cmd->encodedSizes();
        appendCompPartSizeFunc(comp, cmd->name(), "_CmdMinEncodedSize_", sizes.min);
        appendCompPartSizeFunc(comp, cmd->name(), "_CmdMaxEncodedSize_", sizes.max);
    }
    _output->appendEol();
}

void OnboardTypeHeaderGen::appendStatusEncoderPrototypes(const Component* comp)
{
    _output->append("/*status encoders*/\n");
    for (const StatusMsg* msg : comp->statusesRange()) {
        _prototypeGen.appendStatusEncoderFunctionPrototype(comp, msg);
        _output->append(";\n");
    }
    _output->appendEol();
}

void OnboardTypeHeaderGen::appendStatusDecoderPrototypes(const Component* comp)
{
    _output->append("/*status decoders*/\n");
    for (const StatusMsg* msg : comp->statusesRange()) {
        _prototypeGen.appendStatusDecoderFunctionPrototype(comp, msg);
        _output->append(";\n");
    }
    _output->appendEol();
}

void OnboardTypeHeaderGen::appendCompPartSizeFunc(const Component* comp, bmcl::StringView name, bmcl::StringView prefix, std::size_t size)
{
    _output->append("static inline size_t Photon");
    _output->appendWithFirstUpper(comp->name());
    _output->append(prefix);
    _output->appendWithFirstUpper(name);
    _output->append("() { return ");
    _output->appendNumericValue(size);
    _output->append("; }\n");
}

void OnboardTypeHeaderGen::appendStatusMinMaxSizeFuncs(const Component* comp)
{
    _output->append("/*status sizes*/\n");
    for (const StatusMsg* msg : comp->statusesRange()) {
        EncodedSizes sizes = msg->encodedSizes();
        appendCompPartSizeFunc(comp, msg->name(), "_StatusMinEncodedSize_", sizes.min);
        appendCompPartSizeFunc(comp, msg->name(), "_StatusMaxEncodedSize_", sizes.max);
    }
    _output->appendEol();
}

void OnboardTypeHeaderGen::appendEventSenderPrototypes(const Component* comp)
{
    _output->append("/*event senders*/\n");
    TypeReprGen reprGen(_output);
    for (const EventMsg* msg : comp->eventsRange()) {
        _prototypeGen.appendEventEncoderFunctionPrototype(comp, msg, &reprGen);
        _output->append(";\n");
    };
    _output->appendEol();
}

void OnboardTypeHeaderGen::appendEventDecoderPrototypes(const Component* comp)
{
    _output->append("/*event decoders*/\n");
    for (const EventMsg* msg : comp->eventsRange()) {
        if (msg->partsRange().empty()) {
            continue;
        }
        _prototypeGen.appendEventDecoderFunctionPrototype(comp, msg);
        _output->append(";\n");
    };
    _output->appendEol();
}

void OnboardTypeHeaderGen::appendEventMinMaxSizeFuncs(const Component* comp)
{
    _output->append("/*event sizes*/\n");
    for (const EventMsg* msg : comp->eventsRange()) {
        EncodedSizes sizes = msg->encodedSizes();
        appendCompPartSizeFunc(comp, msg->name(), "_EventMinEncodedSize_", sizes.min);
        appendCompPartSizeFunc(comp, msg->name(), "_EventMaxEncodedSize_", sizes.max);
    }
    _output->appendEol();
}

void OnboardTypeHeaderGen::appendCommandPrototypes(const Component* comp)
{
    _output->append("/*cmd handlers*/\n");
    TypeReprGen reprGen(_output);
    for (const Command* cmd : comp->cmdsRange()) {
        _prototypeGen.appendCmdHandlerFunctionProrotype(comp, cmd, &reprGen);
        _output->append(";\n");
    }
    _output->appendEol();
}

void OnboardTypeHeaderGen::appendCommandArgAllocators(const Component* comp)
{
    _output->append("/*cmd arg allocators*/\n");
    TypeReprGen reprGen(_output);
    for (const Command* cmd : comp->cmdsRange()) {
        for (const CmdArgument& arg : cmd->argumentsRange()) {
            if (arg.argPassKind() == CmdArgPassKind::AllocPtr) {
                _prototypeGen.appendCmdArgAllocFunctionPrototype(comp, cmd, arg, &reprGen);
                _output->append(";\n");
            }
        }
    }
    _output->appendEol();
}

void OnboardTypeHeaderGen::appendFunctionPrototype(const Function* func, bmcl::StringView typeName)
{
    const FunctionType* type = func->type();
    bmcl::OptionPtr<const Type> rv = type->returnValue();
    TypeReprGen reprGen(_output);
    if (rv.isSome()) {
        reprGen.genOnboardTypeRepr(rv.unwrap());
        _output->append(' ');
    } else {
        _output->append("void ");
    }
    _output->append("Photon");
    if (typeName != "core") {
        _output->appendWithFirstUpper(typeName);
    }
    _output->append('_');
    _output->appendWithFirstUpper(func->name());
    _output->append('(');

    auto appendSelfArg = [this](bmcl::StringView typeName) {
        _output->append("Photon");
        _output->append(typeName);
        _output->append("* self");
    };

    if (type->selfArgument().isSome()) {
        SelfArgument self = type->selfArgument().unwrap();
        switch(self) {
        case SelfArgument::Reference:
            _output->append("const ");
            appendSelfArg(typeName);
            break;
        case SelfArgument::MutReference:
            appendSelfArg(typeName);
            break;
        case SelfArgument::Value:
            _output->append("Photon");
            _output->append(typeName);
            _output->append(" self");
            break;
        }
        if (type->hasArguments()) {
            _output->append(", ");
        }
    }

    foreachList(type->argumentsRange(), [&](const Field* field) {
        reprGen.genOnboardTypeRepr(field->type(), field->name());
    }, [this](const Field*) {
        _output->append(", ");
    });

    _output->append(");\n");
}

void OnboardTypeHeaderGen::appendCommonIncludePaths()
{
    _output->appendInclude("stdbool.h");
    _output->appendInclude("stddef.h");
    _output->appendInclude("stdint.h");
    _output->appendEol();
}
}
