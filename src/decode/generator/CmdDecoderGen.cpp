/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "decode/generator/CmdDecoderGen.h"
#include "decode/ast/Function.h"
#include "decode/ast/Component.h"
#include "decode/generator/TypeReprGen.h"
#include "decode/generator/FuncPrototypeGen.h"

namespace decode {

CmdDecoderGen::CmdDecoderGen(SrcBuilder* output)
    : _output(output)
    , _inlineInspector(_output)
    , _paramInspector(_output)
{
}

CmdDecoderGen::~CmdDecoderGen()
{
}

void CmdDecoderGen::generateHeader(ComponentMap::ConstRange comps)
{
    (void)comps;
    _output->startIncludeGuard("PRIVATE", "CMD_DECODER");
    _output->appendEol();

    _output->appendOnboardIncludePath("core/Error");
    _output->appendOnboardIncludePath("core/Reader");
    _output->appendOnboardIncludePath("core/Writer");
    _output->appendImplIncludePath("core/Try");
    _output->appendEol();

    _output->startCppGuard();

    appendScriptFunctionPrototype();
    _output->append(";\n\n");
    appendCmdFunctionPrototype();
    _output->append(";\n\n");

    _output->endCppGuard();
    _output->appendEol();
    _output->endIncludeGuard();
}

void CmdDecoderGen::generateSource(ComponentMap::ConstRange comps)
{
    _output->appendOnboardIncludePath("CmdDecoder");
    _output->appendImplIncludePath("core/Logging");
    _output->appendEol();
    _output->append("#define _PHOTON_FNAME \"CmdDecoder.c\"\n\n");

    for (const Component* it : comps) {
        _output->appendModIfdef(it->moduleName());
        _output->appendOnboardComponentInclude(it->moduleName(), ".h");
        _output->appendEndif();
    }

    for (const Component* comp : comps) {
        if (!comp->hasCmds()) {
            continue;
        }
        _output->appendModIfdef(comp->moduleName());
        _output->appendEol();
        for (const Command* cmd : comp->cmdsRange()) {
            generateDecoder(comp, cmd);
            _output->appendEol();
            _output->appendEol();
        }
        _output->appendEndif();
        _output->appendEol();
    }

    generateScriptFunc();
    generateCmdFunc(comps);
    _output->append("\n#undef _PHOTON_FNAME\n");
}

void CmdDecoderGen::appendCmdFunctionPrototype()
{
    _output->append("PhotonError Photon_DeserializeAndExecCmd(uint8_t compNum, uint8_t cmdNum, PhotonReader* src, PhotonWriter* dest)");
}

void CmdDecoderGen::appendScriptFunctionPrototype()
{
    _output->append("PhotonError Photon_ExecScript(PhotonReader* src, PhotonWriter* dest)");
}

void CmdDecoderGen::generateScriptFunc()
{
    appendScriptFunctionPrototype();
    _output->append("\n{\n"
                    "    uint8_t compNum;\n"
                    "    uint8_t cmdNum;\n\n"
                    "    (void)src;\n"
                    "    (void)dest;\n\n"
                    "    while (PhotonReader_ReadableSize(src) != 0) {\n"
                    "        if (PhotonReader_ReadableSize(src) < 2) {\n"
                    "            PHOTON_CRITICAL(\"Not enough data to deserialize cmd header\");\n"
                    "            return PhotonError_NotEnoughData;\n"
                    "        }\n"
                    "        compNum = PhotonReader_ReadU8(src);\n"
                    "        cmdNum = PhotonReader_ReadU8(src);\n"
                    "        PHOTON_TRY(Photon_DeserializeAndExecCmd(compNum, cmdNum, src, dest));\n"
                   );

    _output->append("    }\n    return PhotonError_Ok;\n}\n\n");
}

void CmdDecoderGen::generateCmdFunc(ComponentMap::ConstRange comps)
{
    FuncPrototypeGen prototypeGen(_output);
    appendCmdFunctionPrototype();
    _output->append("\n{\n"
                    "    (void)compNum;\n"
                    "    (void)cmdNum;\n"
                    "    (void)src;\n"
                    "    (void)dest;\n\n"
                    "    switch (compNum) {\n");
    for (const Component* comp : comps) {
        if (!comp->hasCmds()) {
            continue;
        }
        _output->appendModIfdef(comp->moduleName());

        _output->append("    case ");
        _output->appendNumericValue(comp->number());
        _output->append(": {\n");

        _output->append("        switch (cmdNum) {\n");

        for (const Command* cmd : comp->cmdsRange()) {
            (void)cmd;
            _output->append("        case ");
            _output->appendNumericValue(cmd->number());
            _output->append(":\n");
            _output->append("            return ");
            prototypeGen.appendCmdDecoderFunctionName(comp, cmd);
            _output->append("(src, dest);\n");
        }
        _output->append("        default:\n");
        _output->append("            PHOTON_CRITICAL(\"Recieved invalid cmd id\");\n");
        _output->append("            return PhotonError_InvalidCmdId;\n");
        _output->append("        }\n    }\n");
        _output->appendEndif();
    }
    _output->append("    }\n    PHOTON_CRITICAL(\"Recieved invalid component id\");\n");
    _output->append("    return PhotonError_InvalidComponentId;\n}");
}


template <typename C>
void CmdDecoderGen::foreachParam(const Command* func, C&& f)
{
    _paramInspector.reset();
    for (const CmdArgument& arg : func->argumentsRange()) {
        _paramInspector.beginGenericField(arg);
        f(arg, _paramInspector.paramName.view());
        _paramInspector.endField(arg);
    }
}

void CmdDecoderGen::writePointerOp(const Type* type)
{
    const Type* t = type; //HACK: fix Rc::operator->
    switch (type->typeKind()) {
    case TypeKind::Reference:
    case TypeKind::Array:
    case TypeKind::Function:
    case TypeKind::Enum:
    case TypeKind::Builtin:
        break;
    case TypeKind::DynArray:
    case TypeKind::Struct:
    case TypeKind::Variant:
        _output->append("&");
        break;
    case TypeKind::Imported:
        writePointerOp(t->asImported()->link());
        break;
    case TypeKind::Alias:
        writePointerOp(t->asAlias()->alias());
        break;
    case TypeKind::Generic:
        assert(false);
        break;
    case TypeKind::GenericInstantiation:
        writePointerOp(t->asGenericInstantiation()->instantiatedType());
        break;
    case TypeKind::GenericParameter:
        assert(false);
        break;
    }
}

void CmdDecoderGen::writeReturnOp(const Type* type)
{
    const Type* t = type; //HACK: fix Rc::operator->
    switch (type->typeKind()) {
    case TypeKind::Array:
        break;
    case TypeKind::Function:
    case TypeKind::Reference:
    case TypeKind::Enum:
    case TypeKind::Builtin:
    case TypeKind::DynArray:
    case TypeKind::Struct:
    case TypeKind::Variant:
        _output->append("&");
        break;
    case TypeKind::Imported:
        writeReturnOp(t->asImported()->link());
        break;
    case TypeKind::Alias:
        writeReturnOp(t->asAlias()->alias());
        break;
    case TypeKind::Generic:
        assert(false);
        break;
    case TypeKind::GenericInstantiation:
        writeReturnOp(t->asGenericInstantiation()->instantiatedType());
        break;
    case TypeKind::GenericParameter:
        assert(false);
        break;
    }
}

void CmdDecoderGen::generateDecoder(const Component* comp, const Command* cmd)
{
    FuncPrototypeGen prototypeGen(_output);
    const FunctionType* ftype = cmd->type();
    prototypeGen.appendCmdDecoderFunctionPrototype(comp, cmd);
    _output->append("\n{\n");
    _output->append("    PHOTON_DEBUG(\"parsing and executing cmd ");
    _output->append(comp->name());
    _output->append("::");
    _output->append(cmd->name());
    _output->append("\");\n\n");

    TypeReprGen reprGen(_output);
    foreachParam(cmd, [&](const CmdArgument& arg, bmcl::StringView name) {
        _output->append("    ");
        if (arg.argPassKind() == CmdArgPassKind::AllocPtr) {
            Rc<const Type> type = new ReferenceType(ReferenceKind::Pointer, true, const_cast<Type*>(arg.field()->type())); //TODO: avoid allocation
            reprGen.genOnboardTypeRepr(type.get(), name);
            _output->append(" = ");
            prototypeGen.appendCmdArgAllocFunctionName(comp, cmd, arg);
            _output->append("();\n");
        } else {
            reprGen.genOnboardTypeRepr(arg.field()->type(), name);
            _output->append(";\n");
        }
    });

    bmcl::OptionPtr<const Type> rv = ftype->returnValue();
    if (rv.isSome()) {
        _output->append("    ");
        reprGen.genOnboardTypeRepr(rv.unwrap(), "_rv");
        _output->append(";\n");
    }
    _output->appendEol();

    _output->append("    (void)src;\n");
    _output->append("    (void)dest;\n\n");

    _paramInspector.reset();
    _paramInspector.inspect<true, false>(cmd->argumentsRange(), &_inlineInspector);
    _output->appendEol();

    //TODO: gen command call
    _output->append("    PHOTON_TRY_MSG(");
    prototypeGen.appendCmdHandlerFunctionName(comp, cmd);
    _output->append("(");
    foreachParam(cmd, [this](const CmdArgument& arg, bmcl::StringView name) {
        switch (arg.argPassKind()) {
        case CmdArgPassKind::Default:
            writePointerOp(arg.field()->type());
            break;
        case CmdArgPassKind::StackValue:
            break;
        case CmdArgPassKind::StackPtr:
            _output->append("&");
            break;
        case CmdArgPassKind::AllocPtr:
            break;
        };
        _output->append(name);
        _output->append(", ");
    });
    if (rv.isSome()) {
        writeReturnOp(rv.unwrap());
        _output->append("_rv");
    } else if (ftype->hasArguments()) {
        _output->removeFromBack(2);
    }
    _output->append("), \"Failed to exec cmd\");\n\n");

    InlineSerContext ctx;
    if (rv.isSome()) {
        _inlineInspector.inspect<true, true>(rv.unwrap(), ctx, "_rv");
    }

    _output->append("\n    return PhotonError_Ok;\n}");
}
}
